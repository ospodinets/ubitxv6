#include <Arduino.h>

#include "colors.h"
#include "button.h"
#include "encoder.h"
#include "menu.h"
#include "menu_utils.h"
#include "nano_gui.h"
#include "pin_definitions.h"
#include "scratch_space.h"
#include "setup.h"
#include "settings.h"
#include "si5351.h"
#include "tuner.h"
#include "utils.h"

/** Menus
 *  The Radio menus are accessed by tapping on the function button. 
 *  - The main loop() constantly looks for a button press and calls doMenu() when it detects
 *  a function button press. 
 *  - As the encoder is rotated, at every 10th pulse, the next or the previous menu
 *  item is displayed. Each menu item is controlled by it's own function.
 *  - Eache menu function may be called to display itself
 *  - Each of these menu routines is called with a button parameter. 
 *  - The btn flag denotes if the menu itme was clicked on or not.
 *  - If the menu item is clicked on, then it is selected,
 *  - If the menu item is NOT clicked on, then the menu's prompt is to be displayed
 */

static const unsigned int COLOR_TEXT = DISPLAY_WHITE;
static const unsigned int COLOR_BACKGROUND = DISPLAY_BLACK;
static const unsigned int COLOR_TITLE_BACKGROUND = DISPLAY_NAVY;
static const unsigned int COLOR_SETTING_BACKGROUND = DISPLAY_NAVY;
static const unsigned int COLOR_ACTIVE_BORDER = DISPLAY_WHITE;
static const unsigned int COLOR_INACTIVE_BORDER = COLOR_BACKGROUND;

static const unsigned int LAYOUT_TITLE_X = 12;
static const unsigned int LAYOUT_TITLE_Y = 12;
static const unsigned int LAYOUT_TITLE_WIDTH = 296;
static const unsigned int LAYOUT_TITLE_HEIGHT = 35;

static const unsigned int LAYOUT_ITEM_X = 30;
static const unsigned int LAYOUT_ITEM_Y = LAYOUT_TITLE_Y + LAYOUT_TITLE_HEIGHT + 5;
static const unsigned int LAYOUT_ITEM_WIDTH = 260;
static const unsigned int LAYOUT_ITEM_HEIGHT = 30;
static const unsigned int LAYOUT_ITEM_PITCH_Y = LAYOUT_ITEM_HEIGHT + 1;

static const unsigned int LAYOUT_SETTING_REF_VALUE_X = LAYOUT_ITEM_X;
static const unsigned int LAYOUT_SETTING_REF_VALUE_Y = LAYOUT_ITEM_Y + 3*LAYOUT_ITEM_PITCH_Y;
static const unsigned int LAYOUT_SETTING_REF_VALUE_WIDTH = LAYOUT_ITEM_WIDTH;
static const unsigned int LAYOUT_SETTING_REF_VALUE_HEIGHT = LAYOUT_ITEM_HEIGHT;

static const unsigned int LAYOUT_SETTING_VALUE_X = LAYOUT_ITEM_X;
static const unsigned int LAYOUT_SETTING_VALUE_Y = LAYOUT_ITEM_Y + 4*LAYOUT_ITEM_PITCH_Y;
static const unsigned int LAYOUT_SETTING_VALUE_WIDTH = LAYOUT_ITEM_WIDTH;
static const unsigned int LAYOUT_SETTING_VALUE_HEIGHT = LAYOUT_ITEM_HEIGHT;

static const unsigned int LAYOUT_INSTRUCTIONS_TEXT_X = 20;
static const unsigned int LAYOUT_INSTRUCTIONS_TEXT_Y = LAYOUT_ITEM_Y;
static const unsigned int LAYOUT_INSTRUCTIONS_TEXT_WIDTH = LAYOUT_ITEM_WIDTH;
static const unsigned int LAYOUT_INSTRUCTIONS_TEXT_HEIGHT = LAYOUT_SETTING_REF_VALUE_Y - LAYOUT_ITEM_Y - 1;

static const unsigned int LAYOUT_CONFIRM_TEXT_X = 20;
static const unsigned int LAYOUT_CONFIRM_TEXT_Y = LAYOUT_ITEM_Y + 5*LAYOUT_ITEM_PITCH_Y;
static const unsigned int LAYOUT_CONFIRM_TEXT_WIDTH = LAYOUT_ITEM_WIDTH;
static const unsigned int LAYOUT_CONFIRM_TEXT_HEIGHT = LAYOUT_ITEM_HEIGHT;

constexpr char strYes [] PROGMEM = "Yes";
constexpr char strNo [] PROGMEM = "No";
constexpr char strHz [] PROGMEM = "Hz";

void displayDialog(const char* title,
                   const char* instructions){
  displayClear(COLOR_BACKGROUND);
  strncpy_P(b,title,sizeof(b));
  displayText(b, LAYOUT_TITLE_X, LAYOUT_TITLE_Y, LAYOUT_TITLE_WIDTH, LAYOUT_TITLE_HEIGHT, COLOR_TEXT, COLOR_TITLE_BACKGROUND, COLOR_ACTIVE_BORDER);
  strncpy_P(b,instructions,sizeof(b));
  displayText(b, LAYOUT_INSTRUCTIONS_TEXT_X, LAYOUT_INSTRUCTIONS_TEXT_Y, LAYOUT_INSTRUCTIONS_TEXT_WIDTH, LAYOUT_INSTRUCTIONS_TEXT_HEIGHT, COLOR_TEXT, COLOR_BACKGROUND, COLOR_BACKGROUND, TextJustification_e::Left);
}

struct SettingScreen_t {
  const char* const Title;
  const char* const AdditionalText;
  const uint16_t KnobDivider;
  const int16_t StepSize;//int so that it can be negative
  void (*Initialize)(long int* start_value_out);
  void (*Validate)(const long int candidate_value_in, long int* validated_value_out);
  void (*OnValueChange)(const long int new_value, char* buff_out, const size_t buff_out_size);
  void (*Finalize)(const long int final_value);
  void (*TouchNext)(const long int current_value, long int* next_value_out);
};

void drawSetting(const SettingScreen_t* const screen)
{
  displayDialog(screen->Title,
                screen->AdditionalText);
}

//State variables for settings
int32_t setupMenuRawValue = 0;
int32_t setupMenuLastValue = 0;
const SettingScreen_t* activeSettingP;
char setupMenuDisplayedValue[30];
bool setupMenuSettingFinished;

static Style settingValueInactiveStyle = {
  COLOR_TEXT, COLOR_TITLE_BACKGROUND, COLOR_BACKGROUND,
  TextJustification_e::Center, FontFamily_e::Regular, 1
};
static Style settingSaveInactiveStyle = {
  COLOR_TEXT, COLOR_BACKGROUND, COLOR_ACTIVE_BORDER,
  TextJustification_e::Center, FontFamily_e::Regular, 1
};

void settingValueText(char* text_out, const uint16_t max_text_size)
{
  if(0 == max_text_size){
    return;
  }
  strncpy(text_out,setupMenuDisplayedValue,max_text_size);
  text_out[max_text_size - 1] = '\0';
}

ButtonStatus_e settingButtonStatus()
{
  return ButtonStatus_e::Stateless;
}

void advanceSettingValue();
void saveSettingValue();

static Button settingValueButton = {
  LAYOUT_SETTING_VALUE_X, LAYOUT_SETTING_VALUE_Y,
  LAYOUT_SETTING_VALUE_WIDTH, LAYOUT_SETTING_VALUE_HEIGHT,
  nullptr, settingValueText, settingButtonStatus, advanceSettingValue,
  &settingValueInactiveStyle, &settingValueInactiveStyle
};
static Button settingSaveButton = {
  LAYOUT_CONFIRM_TEXT_X, LAYOUT_CONFIRM_TEXT_Y,
  LAYOUT_CONFIRM_TEXT_WIDTH, LAYOUT_CONFIRM_TEXT_HEIGHT,
  "Push Tune to Save", nullptr,
  settingButtonStatus, saveSettingValue,
  &settingSaveInactiveStyle, &settingSaveInactiveStyle
};
static const Button* const settingButtons[] = {
  &settingValueButton, &settingSaveButton
};

void refreshSettingValue(const SettingScreen_t& screen, const long int value)
{
  screen.OnValueChange(value,b,sizeof(b));
  strncpy(setupMenuDisplayedValue,b,sizeof(setupMenuDisplayedValue));
  setupMenuDisplayedValue[sizeof(setupMenuDisplayedValue) - 1] = '\0';
  settingValueButton.invalidate();
  settingValueButton.draw();
  setupMenuLastValue = value;
}

void activateSetting(SettingScreen_t* new_setting_P);

void initSetting();
MenuReturn_e runSetting(const ButtonPress_e tuner_button,
                        const ButtonPress_e touch_button,
                        const Point touch_point,
                        const int16_t knob);
Menu_t setupMenuActiveSettingMenu = {
  initSetting,
  runSetting,
  nullptr
};

void initSetting()
{
  if(nullptr == activeSettingP){
    return;
  }
  drawSetting(activeSettingP);
  activeSettingP->Initialize(&setupMenuLastValue);
  setupMenuSettingFinished = false;
  activeSettingP->OnValueChange(setupMenuLastValue,b,sizeof(b));
  strncpy(setupMenuDisplayedValue,b,sizeof(setupMenuDisplayedValue));
  setupMenuDisplayedValue[sizeof(setupMenuDisplayedValue) - 1] = '\0';
  settingValueButton.invalidate();
  settingValueButton.draw();
  displayText(b, LAYOUT_SETTING_REF_VALUE_X, LAYOUT_SETTING_REF_VALUE_Y, LAYOUT_SETTING_REF_VALUE_WIDTH, LAYOUT_SETTING_REF_VALUE_HEIGHT, COLOR_SETTING_BACKGROUND, COLOR_BACKGROUND, COLOR_BACKGROUND);
  setupMenuRawValue = setupMenuLastValue * (int32_t)activeSettingP->KnobDivider;
  settingSaveButton.invalidate();
  settingSaveButton.draw();
}

void saveSettingValue()
{
  if(nullptr == activeSettingP){
    return;
  }
  activeSettingP->Finalize(setupMenuLastValue);
  setupMenuSettingFinished = true;
}

void advanceSettingValue()
{
  if(nullptr == activeSettingP){
    return;
  }
  
  long int candidate_value;
  if(nullptr != activeSettingP->TouchNext){
    activeSettingP->TouchNext(setupMenuLastValue,&candidate_value);
  }
  else{
    const long int direction = (0 > activeSettingP->StepSize) ? -1 : 1;
    candidate_value = setupMenuLastValue + direction * abs(activeSettingP->StepSize);
  }

  long int value = 0;
  activeSettingP->Validate(candidate_value,&value);
  if(value != setupMenuLastValue){
    refreshSettingValue(*activeSettingP,value);
    setupMenuRawValue = value * (int32_t)activeSettingP->KnobDivider;
  }
}

MenuReturn_e runSetting(const ButtonPress_e tuner_button,
                        const ButtonPress_e touch_button,
                        const Point touch_point,
                        const int16_t knob)
{
  if(nullptr == activeSettingP){
    return MenuReturn_e::ExitedRedraw;
  }

  if(ButtonPress_e::NotPressed != tuner_button){
    //Long or short press, we do the same thing
    activeSettingP->Finalize(setupMenuLastValue);
    return MenuReturn_e::ExitedRedraw;
  }

  if(ButtonPress_e::NotPressed != touch_button){
    const Button* button = findPressedButton(settingButtons,
                                              sizeof(settingButtons)/sizeof(settingButtons[0]),
                                              touch_point);
    if(nullptr != button){
      button->select();
      if(setupMenuSettingFinished){
        return MenuReturn_e::ExitedRedraw;
      }
    }
    return MenuReturn_e::StillActive;
  }

  if(0 != knob){
    setupMenuRawValue += knob * activeSettingP->StepSize;

    const int32_t candidate_value = setupMenuRawValue / (int32_t)activeSettingP->KnobDivider;
    int32_t value = 0;
    activeSettingP->Validate(candidate_value,&value);

    //If we're going out of bounds, prevent the raw value from going too far out
    if(candidate_value != value){
      setupMenuRawValue = value * (int32_t)activeSettingP->KnobDivider;
    }

    if(value != setupMenuLastValue){
      refreshSettingValue(*activeSettingP,value);
    }
  }

  return MenuReturn_e::StillActive;
}

void activateSetting(const SettingScreen_t *const new_setting_P)
{
  activeSettingP = new_setting_P;
  enterSubmenu(&setupMenuActiveSettingMenu);
}

//Local Oscillator
void ssLocalOscInitialize(long int* start_value_out){
  {
    uint32_t freq = GetActiveVfoFreq();
    freq = (freq/1000L) * 1000L;//round off the current frequency the nearest kHz
    setFrequency(freq);
  }
  *start_value_out = globalSettings.oscillatorCal;
}
void ssLocalOscValidate(const long int candidate_value_in, long int* validated_value_out)
{
  *validated_value_out = candidate_value_in;//No check - allow anything
}
void ssLocalOscChange(const long int new_value, char* buff_out, const size_t buff_out_size)
{
  si5351_set_calibration(new_value);
  setFrequency(GetActiveVfoFreq());
  const long int u = abs(new_value);
  if(new_value != u){
    strncpy_P(buff_out,(const char*)F("-"),buff_out_size);
    ++buff_out;
  }
  formatFreq(u,buff_out,buff_out_size - strlen(buff_out));
  strncat_P(buff_out,strHz,buff_out_size - strlen(buff_out));
}
void ssLocalOscFinalize(const long int final_value)
{
  globalSettings.oscillatorCal = final_value;
  SaveSettingsToEeprom();
  si5351_set_calibration(globalSettings.oscillatorCal);
  setFrequency(GetActiveVfoFreq());
}
const char SS_LOCAL_OSC_T [] PROGMEM = "Local Oscillator";
const char SS_LOCAL_OSC_A [] PROGMEM = "Tune so that the dial displays\na known freq, then tune here\nuntil the signal is zerobeat";
const SettingScreen_t ssLocalOsc PROGMEM = {
  SS_LOCAL_OSC_T,
  SS_LOCAL_OSC_A,
  1,
  50,
  ssLocalOscInitialize,
  ssLocalOscValidate,
  ssLocalOscChange,
  ssLocalOscFinalize,
  nullptr
};
void runLocalOscSetting(){activateSetting(&ssLocalOsc);}

//BFO
void ssBfoInitialize(long int* start_value_out){
  si5351bx_setfreq(0, globalSettings.usbCarrierFreq);
  *start_value_out = globalSettings.usbCarrierFreq;
}
void ssBfoValidate(const long int candidate_value_in, long int* validated_value_out)
{
  *validated_value_out = LIMIT(candidate_value_in,11048000L,11060000L);
}
void ssBfoChange(const long int new_value, char* buff_out, const size_t buff_out_size)
{
  globalSettings.usbCarrierFreq = new_value;
  setFrequency(GetActiveVfoFreq());
  si5351bx_setfreq(0, new_value);
  formatFreq(new_value,buff_out,buff_out_size);
  strncat_P(buff_out,strHz,buff_out_size - strlen(buff_out));
}
void ssBfoFinalize(const long int final_value)
{
  globalSettings.usbCarrierFreq = final_value;
  SaveSettingsToEeprom();
  si5351bx_setfreq(0, globalSettings.usbCarrierFreq);
  setFrequency(GetActiveVfoFreq());
}
const char SS_BFO_T [] PROGMEM = "Beat Frequency Osc (BFO)";
const char SS_BFO_A [] PROGMEM = "Tune until the audio is\nbetween 300-3000Hz";
const SettingScreen_t ssBfo PROGMEM = {
  SS_BFO_T,
  SS_BFO_A,
  1,
  -50,//Negative to make dial more intuitive: turning clockwise increases the perceived audio frequency
  ssBfoInitialize,
  ssBfoValidate,
  ssBfoChange,
  ssBfoFinalize,
  nullptr
};
void runBfoSetting(){activateSetting(&ssBfo);}

//CW Tone
void ssCwToneInitialize(long int* start_value_out)
{
  *start_value_out = globalSettings.cwSideToneFreq;
}
void ssCwToneValidate(const long int candidate_value_in, long int* validated_value_out)
{
  *validated_value_out = LIMIT(candidate_value_in,100,2000);
}
void ssCwToneChange(const long int new_value, char* buff_out, const size_t buff_out_size)
{
  globalSettings.cwSideToneFreq = new_value;
  startCwPractice(globalSettings.cwSideToneFreq, globalSettings.cwDitDurationMs);
  ltoa(globalSettings.cwSideToneFreq,buff_out,10);
  strncat_P(buff_out,strHz,buff_out_size - strlen(buff_out));
}
void ssCwToneFinalize(const long int final_value)
{
  stopCwPractice();
  globalSettings.cwSideToneFreq = final_value;
  SaveSettingsToEeprom();
}
const char SS_CW_TONE_T [] PROGMEM = "Tone";
const char SS_CW_TONE_A [] PROGMEM = "Select a frequency that\nCW mode to tune for";
const SettingScreen_t ssTone PROGMEM = {
  SS_CW_TONE_T,
  SS_CW_TONE_A,
  1,
  1,
  ssCwToneInitialize,
  ssCwToneValidate,
  ssCwToneChange,
  ssCwToneFinalize,
  nullptr
};
void runToneSetting(){activateSetting(&ssTone);}

//CW Switch Delay
void ssCwSwitchDelayInitialize(long int* start_value_out)
{
  *start_value_out = globalSettings.cwActiveTimeoutMs;
}
void ssCwSwitchDelayValidate(const long int candidate_value_in, long int* validated_value_out)
{
  *validated_value_out = LIMIT(candidate_value_in,100,1000);
}
void ssCwSwitchDelayChange(const long int new_value, char* buff_out, const size_t buff_out_size)
{
  ltoa(new_value,buff_out,10);
  strncat_P(buff_out,(const char*)F("ms"),buff_out_size - strlen(buff_out));
}
void ssCwSwitchDelayFinalize(const long int final_value)
{
  globalSettings.cwActiveTimeoutMs = final_value;
  SaveSettingsToEeprom();
}
const char SS_CW_SWITCH_T [] PROGMEM = "Tx to Rx Delay";
const char SS_CW_SWITCH_A [] PROGMEM = "How long to wait before\nswitching from TX to RX when\nin CW mode";
const SettingScreen_t ssCwSwitchDelay PROGMEM = {
  SS_CW_SWITCH_T,
  SS_CW_SWITCH_A,
  1,
  10,
  ssCwSwitchDelayInitialize,
  ssCwSwitchDelayValidate,
  ssCwSwitchDelayChange,
  ssCwSwitchDelayFinalize,
  nullptr
};
void runCwSwitchDelaySetting(){activateSetting(&ssCwSwitchDelay);}

//CW Keyer
void ssKeyerInitialize(long int* start_value_out)
{
  *start_value_out = globalSettings.keyerMode;
}
void ssKeyerValidate(const long int candidate_value_in, long int* validated_value_out)
{
  *validated_value_out = LIMIT(candidate_value_in,(uint8_t)KeyerMode_e::KEYER_STRAIGHT,(uint8_t)KeyerMode_e::KEYER_IAMBIC_B);
}
void ssKeyerTouchNext(const long int current_value, long int* next_value_out)
{
  const long int last_value = (uint8_t)KeyerMode_e::KEYER_IAMBIC_B;
  *next_value_out = current_value >= last_value
                  ? (uint8_t)KeyerMode_e::KEYER_STRAIGHT
                  : current_value + 1;
}
void ssKeyerChange(const long int new_value, char* buff_out, const size_t buff_out_size)
{
  if(KeyerMode_e::KEYER_STRAIGHT == new_value){
    strncpy_P(buff_out,(const char*)F("Hand Key"),buff_out_size);
  }
  else if(KeyerMode_e::KEYER_IAMBIC_A == new_value){
    strncpy_P(buff_out,(const char*)F("Iambic A"),buff_out_size);
  }
  else{
    strncpy_P(buff_out,(const char*)F("Iambic B"),buff_out_size);
  }
  enc_read();//Consume any rotations during morse playback
}
void ssKeyerFinalize(const long int final_value)
{
  globalSettings.keyerMode = (KeyerMode_e)final_value;
  SaveSettingsToEeprom();
}
const char SS_KEYER_T [] PROGMEM = "Keyer Type";
const char SS_KEYER_A [] PROGMEM = "Select which type of keyer\nor paddle is being used";
const SettingScreen_t ssKeyer PROGMEM = {
  SS_KEYER_T,
  SS_KEYER_A,
  10,
  1,
  ssKeyerInitialize,
  ssKeyerValidate,
  ssKeyerChange,
  ssKeyerFinalize,
  ssKeyerTouchNext
};
void runKeyerSetting(){activateSetting(&ssKeyer);}


void ssToggleTouchNext(const long int current_value, long int* next_value_out)
{
  *next_value_out = current_value ? 0 : 1;
}

//CW Speed
void ssCwSpeedInitialize(long int* start_value_out)
{
  *start_value_out = 1200L/globalSettings.cwDitDurationMs;
}
void ssCwSpeedValidate(const long int candidate_value_in, long int* validated_value_out)
{
  *validated_value_out = LIMIT(candidate_value_in,1,100);
}
void ssCwSpeedChange(const long int new_value, char* buff_out, const size_t /*buff_out_size*/)
{
  ltoa(new_value, buff_out, 10);
  startCwPractice(globalSettings.cwSideToneFreq, 1200L / new_value);
  enc_read();//Consume any rotations during morse playback
}
void ssCwSpeedFinalize(const long int final_value)
{
  stopCwPractice();
  globalSettings.cwDitDurationMs = 1200L/final_value;
  SaveSettingsToEeprom();
}
const char SS_CW_SPEED_T [] PROGMEM = "Play Speed";
const char SS_CW_SPEED_A [] PROGMEM = "Speed to play CW characters";
const SettingScreen_t ssCwSpeed PROGMEM = {
  SS_CW_SPEED_T,
  SS_CW_SPEED_A,
  5,
  1,
  ssCwSpeedInitialize,
  ssCwSpeedValidate,
  ssCwSpeedChange,
  ssCwSpeedFinalize,
  nullptr
};
void runCwSpeedSetting(){activateSetting(&ssCwSpeed);}

//Reset all settings
void ssResetAllInitialize(long int* start_value_out)
{
  *start_value_out = 0;//Default to NOT resetting
}
void ssResetAllValidate(const long int candidate_value_in, long int* validated_value_out)
{
  *validated_value_out = LIMIT(candidate_value_in,0,1);
}
void ssResetAllChange(const long int new_value, char* buff_out, const size_t buff_out_size)
{
  if(new_value){
    strncpy_P(buff_out,strYes,buff_out_size);
  }
  else{
    strncpy_P(buff_out,strNo,buff_out_size);
  }
  enc_read();//Consume any rotations during morse playback
}
void ssResetAllFinalize(const long int final_value)
{
  if(final_value){
    LoadDefaultSettings();
    SaveSettingsToEeprom();
    setup();
  }
}
const char SS_RESET_ALL_T [] PROGMEM = "Reset All";
const char SS_RESET_ALL_A [] PROGMEM = "Resets all calibrations and\nsettings to their default\nvalues";
const SettingScreen_t ssResetAll PROGMEM = {
  SS_RESET_ALL_T,
  SS_RESET_ALL_A,
  20,
  1,
  ssResetAllInitialize,
  ssResetAllValidate,
  ssResetAllChange,
  ssResetAllFinalize,
  ssToggleTouchNext
};
void runResetAllSetting(){activateSetting(&ssResetAll);}

struct MenuItem_t {
  const char* const ItemName;
  void (*OnSelect)();
};

void initSetupMenu(const MenuItem_t* const menu_items,
                   const uint16_t num_items);
MenuReturn_e runSetupMenu(const MenuItem_t* const menu_items,
                          const uint16_t num_items,
                          const ButtonPress_e tuner_button,
                          const ButtonPress_e touch_button,
                          const Point touch_point,
                          const int16_t knob);

#define GENERATE_MENU_T(menu_name) \
          void initSetupMenu##menu_name(void)\
          {\
            initSetupMenu(menuItems##menu_name,sizeof(menuItems##menu_name)/sizeof(menuItems##menu_name[0]));\
          }\
          MenuReturn_e runSetupMenu##menu_name(const ButtonPress_e tuner_button,\
                                            const ButtonPress_e touch_button,\
                                            const Point touch_point,\
                                            const int16_t knob)\
          {\
            return runSetupMenu(menuItems##menu_name,\
                                sizeof(menuItems##menu_name)/sizeof(menuItems##menu_name[0]),\
                                tuner_button,\
                                touch_button,\
                                touch_point,\
                                knob\
                                );\
          }\
          Menu_t setupMenu##menu_name = {\
            initSetupMenu##menu_name,\
            runSetupMenu##menu_name,\
            nullptr\
          };\
          void run##menu_name##Menu(void)\
          {\
            enterSubmenu(&setupMenu##menu_name);\
          }\

const char MI_TOUCH [] PROGMEM = "Touch Screen";
void setupTouchSetting();

const char MT_CAL [] PROGMEM = "Calibrations";
const MenuItem_t menuItemsCalibration [] PROGMEM {
  {MT_CAL,nullptr},//Title
  {SS_LOCAL_OSC_T,runLocalOscSetting},
  {SS_BFO_T,runBfoSetting},
  {MI_TOUCH,setupTouchSetting},
};
GENERATE_MENU_T(Calibration);

void setupTouchSetting(){
  setupTouch();
  initSetupMenuCalibration();
}

const char MT_CW [] PROGMEM = "CW Setup";
const MenuItem_t menuItemsCw [] PROGMEM {
  {MT_CW,nullptr},//Title
  {SS_CW_TONE_T,runToneSetting},
  {SS_CW_SWITCH_T,runCwSwitchDelaySetting},
  {SS_KEYER_T,runKeyerSetting},
  {SS_CW_SPEED_T,runCwSpeedSetting},
};
GENERATE_MENU_T(Cw);

const char MT_SETTINGS [] PROGMEM = "Settings";
const MenuItem_t menuItemsSetupRoot [] PROGMEM {
  {MT_SETTINGS,nullptr},//Title
  {MT_CAL,runCalibrationMenu},
  {MT_CW,runCwMenu},
  {SS_RESET_ALL_T,runResetAllSetting},
};
GENERATE_MENU_T(SetupRoot);
Menu_t *const setupMenu = &setupMenuSetupRoot;

const char MI_EXIT [] PROGMEM = "Exit";
const MenuItem_t exitMenu PROGMEM = {MI_EXIT,nullptr};

int16_t setupMenuSelector = 0;
const MenuItem_t* activeSetupMenuItems;
uint16_t activeSetupMenuItemCount;
bool setupMenuExitSelected;

uint8_t setupMenuSelectedButton()
{
  return setupMenuSelector / MENU_KNOB_COUNTS_PER_ITEM;
}

void setupMenuButtonText(uint8_t index, char* text_out, const uint16_t max_text_size)
{
  if(0 == max_text_size){
    return;
  }

  if(index < activeSetupMenuItemCount - 1){
    MenuItem_t mi = {"",nullptr};
    memcpy_P(&mi,&activeSetupMenuItems[index + 1],sizeof(mi));
    strncpy_P(text_out,mi.ItemName,max_text_size);
    text_out[max_text_size - 1] = '\0';
  }
  else{
    strncpy_P(text_out,MI_EXIT,max_text_size);
    text_out[max_text_size - 1] = '\0';
  }
}

void setupMenuButtonSelect(uint8_t index)
{
  if(index >= activeSetupMenuItemCount - 1){
    setupMenuExitSelected = true;
    return;
  }

  MenuItem_t mi = {"",nullptr};
  memcpy_P(&mi,&activeSetupMenuItems[index + 1],sizeof(mi));
  if(nullptr == mi.OnSelect){
    setupMenuExitSelected = true;
  }
  else{
    mi.OnSelect();
  }
}

ButtonStatus_e setupMenuButtonStatus(uint8_t index)
{
  return index == setupMenuSelectedButton()
       ? ButtonStatus_e::Active
       : ButtonStatus_e::Inactive;
}

#define SETUP_MENU_BUTTON_GENERATE(index) \
  void setupMenuButtonText##index(char* text_out, const uint16_t max_text_size){setupMenuButtonText(index,text_out,max_text_size);}\
  ButtonStatus_e setupMenuButtonStatus##index(){return setupMenuButtonStatus(index);}\
  void setupMenuButtonSelect##index(){setupMenuButtonSelect(index);}\
  static Button setupMenuButton##index = {\
    LAYOUT_ITEM_X, LAYOUT_ITEM_Y + index*LAYOUT_ITEM_PITCH_Y,\
    LAYOUT_ITEM_WIDTH, LAYOUT_ITEM_HEIGHT,\
    nullptr, setupMenuButtonText##index,\
    setupMenuButtonStatus##index, setupMenuButtonSelect##index,\
    &setupMenuInactiveStyle, &setupMenuActiveStyle\
  }

static Style setupMenuInactiveStyle = {
  COLOR_TEXT, COLOR_BACKGROUND, COLOR_INACTIVE_BORDER,
  TextJustification_e::Left, FontFamily_e::Regular, 1
};
static Style setupMenuActiveStyle = {
  COLOR_TEXT, COLOR_TITLE_BACKGROUND, COLOR_ACTIVE_BORDER,
  TextJustification_e::Left, FontFamily_e::Regular, 1
};

SETUP_MENU_BUTTON_GENERATE(0);
SETUP_MENU_BUTTON_GENERATE(1);
SETUP_MENU_BUTTON_GENERATE(2);
SETUP_MENU_BUTTON_GENERATE(3);
SETUP_MENU_BUTTON_GENERATE(4);
SETUP_MENU_BUTTON_GENERATE(5);

const Button* const setupMenuButtons[] = {
  &setupMenuButton0, &setupMenuButton1, &setupMenuButton2,
  &setupMenuButton3, &setupMenuButton4, &setupMenuButton5
};

void drawSetupMenuButton(uint8_t index)
{
  setupMenuButtons[index]->draw();
}

void drawMenu(const MenuItem_t* const items, const uint16_t num_items)
{
  displayClear(COLOR_BACKGROUND);
  MenuItem_t mi = {"",nullptr};
  memcpy_P(&mi,&items[0],sizeof(mi));
  strncpy_P(b,mi.ItemName,sizeof(b));
  displayText(b, LAYOUT_TITLE_X, LAYOUT_TITLE_Y, LAYOUT_TITLE_WIDTH, LAYOUT_TITLE_HEIGHT, COLOR_TEXT, COLOR_TITLE_BACKGROUND, COLOR_ACTIVE_BORDER);
  for(uint8_t i = 0; i < num_items; ++i){
    drawSetupMenuButton(i);
  }
}

void setSetupMenuSelection(uint8_t new_index)
{
  const uint8_t old_index = setupMenuSelectedButton();
  if(old_index == new_index){
    return;
  }

  setupMenuSelector = new_index * MENU_KNOB_COUNTS_PER_ITEM;
  drawSetupMenuButton(old_index);
  drawSetupMenuButton(new_index);
}

void initSetupMenu(const MenuItem_t* const menu_items,
                   const uint16_t num_items)
{
  activeSetupMenuItems = menu_items;
  activeSetupMenuItemCount = num_items;
  setupMenuSelector = 0;
  setupMenuExitSelected = false;
  drawMenu(menu_items,num_items);
}

MenuReturn_e runSetupMenu(const MenuItem_t* const menu_items,
                          const uint16_t num_items,
                          const ButtonPress_e tuner_button,
                          const ButtonPress_e touch_button,
                          const Point touch_point,
                          const int16_t knob)
{
  (void)menu_items;
  if(setupMenuExitSelected){
    return MenuReturn_e::ExitedRedraw;
  }

  if(ButtonPress_e::NotPressed != tuner_button){
    setupMenuButtons[setupMenuSelectedButton()]->select();
  }
  else if(ButtonPress_e::NotPressed != touch_button){
    const Button* button = findPressedButton(setupMenuButtons,(uint8_t)num_items,touch_point);
    if(nullptr != button){
      uint8_t index = 0;
      while(setupMenuButtons[index] != button){
        ++index;
      }
      setSetupMenuSelection(index);
      button->select();
    }
  }
  else if(0 != knob){
    const uint8_t old_index = setupMenuSelectedButton();
    setupMenuSelector = LIMIT(setupMenuSelector + knob,
                              0,
                              (int16_t)(num_items*MENU_KNOB_COUNTS_PER_ITEM - 1));
    const uint8_t new_index = setupMenuSelectedButton();
    if(old_index != new_index){
      drawSetupMenuButton(old_index);
      drawSetupMenuButton(new_index);
    }
  }

  return setupMenuExitSelected ? MenuReturn_e::ExitedRedraw : MenuReturn_e::StillActive;
}
