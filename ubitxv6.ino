 /**
 * This source file is under General Public License version 3.
 * 
 * This verision uses a built-in Si5351 library
 * Most source code are meant to be understood by the compilers and the computers. 
 * Code that has to be hackable needs to be well understood and properly documented. 
 * Donald Knuth coined the term Literate Programming to indicate code that is written be 
 * easily read and understood.
 * 
 * The Raduino is a small board that includes the Arduin Nano, a TFT display and
 * an Si5351a frequency synthesizer. This board is manufactured by HF Signals Electronics Pvt Ltd
 * 
 * To learn more about Arduino you may visit www.arduino.cc. 
 * 
 * The Arduino works by starts executing the code in a function called setup() and then it 
 * repeatedly keeps calling loop() forever. All the initialization code is kept in setup()
 * and code to continuously sense the tuning knob, the function button, transmit/receive,
 * etc is all in the loop() function. If you wish to study the code top down, then scroll
 * to the bottom of this file and read your way up.
 * 
 * Below are the libraries to be included for building the Raduino 
 * The EEPROM library is used to store settings like the frequency memory, caliberation data, etc.
 *
 *  The main chip which generates upto three oscillators of various frequencies in the
 *  Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet 
 *  from www.silabs.com although, strictly speaking it is not a requirment to understand this code. 
 *  Instead, you can look up the Si5351 library written by xxx, yyy. You can download and 
 *  install it from www.url.com to complile this file.
 *  The Wire.h library is used to talk to the Si5351 and we also declare an instance of 
 *  Si5351 object to control the clocks.
 */
#include <Wire.h>
#include "encoder.h"
#include "events.h"
#include "menu.h"
#include "menu_main.h"
#include "pin_definitions.h"
#include "ptt.h"
#include "push_button.h"
#include "nano_gui.h"
#include "settings.h"
#include "setup.h"
#include "si5351.h"
#include "touch.h"
#include "tuner.h"
#include "ui_touch.h"

/**
 * The Arduino, unlike C/C++ on a regular computer with gigabytes of RAM, has very little memory.
 * We have to be very careful with variables that are declared inside the functions as they are 
 * created in a memory region called the stack. The stack has just a few bytes of space on the Arduino
 * if you declare large strings inside functions, they can easily exceed the capacity of the stack
 * and mess up your programs. 
 * We circumvent this by declaring a few global buffers as kitchen counters where we can 
 * slice and dice our strings for display.
 */
char b[128];
char c[30];

/**
 * Basic User Interface Routines. These check the front panel for any activity
 */

//Set while TX blocks the user interface, so the controls are re-armed when it ends
static bool userInputSuspended = false;

/**
 * The PTT is handled only if we are not in cw mode, where the keyer reads the
 * pin directly so that it can be used as a straight key.
 * If the PTT is pressed, we shift to the ritbase if the rit was on
 * flip the T/R line to T and update the display to denote transmission
 */
void handlePttEvent(const Event& event){
  if(TuningMode_e::TUNE_CW == globalSettings.tuningMode
     || globalSettings.cwExpirationTimeMs > 0){
    return;
  }

  if(event.pressed && !globalSettings.txActive){
    userInputSuspended = true;
    startTx(TuningMode_e::TUNE_SSB);
  }
  else if(!event.pressed && globalSettings.txActive){
    stopTx();
  }
}

/**
 * The event queue is full only if the loop was stalled, in which case a PTT edge
 * may have been lost. Resynchronize from the pin so TX can't get stuck.
 */
void resyncPttIfEventsDropped(){
  static uint16_t seenDropped = 0;
  const uint16_t dropped = eventDroppedCount();
  if(dropped != seenDropped){
    seenDropped = dropped;
    Event event = {};
    event.type = EventType::PttChanged;
    event.pressed = IsPttPressed();
    handlePttEvent(event);
  }
}

/**
 * The settings are read from EEPROM. The first time around, the values may not be 
 * present or out of range, in this case, some intelligent defaults are copied into the 
 * variables.
 */
void initSettings(){
  LoadDefaultSettings();
  LoadSettingsFromEeprom();
}

void initPorts(){

  analogReference(AR_DEFAULT);

  //??
  enc_setup();
  
  initTunerButton();
  initPtt();
  pinMode(PIN_KEYER_DIT, INPUT_PULLUP);
  pinMode(PIN_KEYER_DAH, INPUT_PULLUP);

  pinMode(PIN_CW_TONE, OUTPUT);
  digitalWrite(PIN_CW_TONE, 0);
  
  pinMode(PIN_TX_RXn,OUTPUT);
  digitalWrite(PIN_TX_RXn, 0);

  pinMode(PIN_TX_LPF_A, OUTPUT);
  pinMode(PIN_TX_LPF_B, OUTPUT);
  pinMode(PIN_TX_LPF_C, OUTPUT);
  digitalWrite(PIN_TX_LPF_A, 0);
  digitalWrite(PIN_TX_LPF_B, 0);
  digitalWrite(PIN_TX_LPF_C, 0);

  pinMode(PIN_CW_KEY, OUTPUT);
  digitalWrite(PIN_CW_KEY, 0);
}

/**
 * Ignores the touch screen and tuner button until they have been released,
 * and forgets any partially detected press. Queued events are left alone.
 */
void resetUserInput(){
  tunerButtonReset();
  touchReset();
}

/**
 * Samples the polled front panel controls, then dispatches every queued event:
 * PTT events control TX, and the others go to the active menu, unless we are transmitting.
 * The knob is handed to the first menu run only, and the menu is run at least
 * once per call so it can keep its display up to date.
 */
void runUserInput(){
  if(!globalSettings.txActive){
    const uint32_t now = millis();
    tunerButtonPoll(now);
    touchPoll(now);
  }

  int16_t knob = globalSettings.txActive ? 0 : enc_read();
  bool menu_run = false;
  Event event;
  while(eventPop(&event)){
    ButtonPress_e tuner_button = ButtonPress_e::NotPressed;
    ButtonPress_e touch_button = ButtonPress_e::NotPressed;
    Point touch_point = {0, 0};
    switch(event.type){
      case EventType::PttChanged:
        handlePttEvent(event);
        continue;
      case EventType::TunerPress:
        tuner_button = event.press;
        break;
      case EventType::TouchPress:
        touch_button = event.press;
        touch_point = event.point;
        break;
    }
    if(globalSettings.txActive){
      continue;//Don't run menus when transmitting
    }
    runActiveMenu(tuner_button,touch_button,touch_point,knob);
    knob = 0;
    menu_run = true;
  }

  if(!menu_run && !globalSettings.txActive){
    const Point no_touch = {0, 0};
    runActiveMenu(ButtonPress_e::NotPressed,ButtonPress_e::NotPressed,no_touch,knob);
  }
}

/**
 * Shows a settings screen and runs the menus until the user has finished with it.
 * Only for use during setup(), where the main loop is not running yet.
 */
void runSettingScreenToCompletion(void (*const show_setting)()){
  eventClearAll();
  resetUserInput();
  show_setting();
  while(nullptr != rootMenu->active_submenu){
    runUserInput();
  }
}

void setup()
{
  Serial.begin(38400);
  Serial.flush();

  initSettings();
  displayInit();
  initTouch();
  initPorts();
  initOscillators();
  setFrequency(globalSettings.vfoA.frequency);

  //Run initial calibration routine if button is pressed during power up
  if(IsButtonPressed()){
    WaitForTunerButtonRelease();//Consume the press so that it isn't seen as a menu action
    LoadDefaultSettings();
    setupTouch();
    SetActiveVfoMode(VfoMode_e::VFO_MODE_USB);
    setFrequency(10000000L);
    runSettingScreenToCompletion(runLocalOscSetting);
    SetActiveVfoMode(VfoMode_e::VFO_MODE_LSB);
    setFrequency(7100000L);
    runSettingScreenToCompletion(runBfoSetting);
  }

  eventClearAll();
  resetUserInput();
  rootMenu->initMenu();
  enablePttEvents();//After setup, so PTT can't start TX during calibration
}


/**
 * The loop checks for keydown, ptt, function button and tuning.
 */

void loop(){
  if(TuningMode_e::TUNE_CW == globalSettings.tuningMode){
    cwKeyer();
  }

  if(userInputSuspended && !globalSettings.txActive){
    //Anything pressed during TX must not act as a menu input afterwards
    resetUserInput();
    userInputSuspended = false;
  }

  resyncPttIfEventsDropped();
  runUserInput();

  if(globalSettings.txActive){
    stopCwPractice();
    userInputSuspended = true;
    return;
  }

  updateCwPractice();
}
