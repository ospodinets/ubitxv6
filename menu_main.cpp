#include "menu_main.h"
#include "menu_main_buttons.h"

#include <Arduino.h>

#include "button.h"
#include "color_theme.h"
#include "menu_utils.h"
#include "morse.h"
#include "nano_gui.h"
#include "scratch_space.h"
#include "settings.h"
#include "tuner.h"//THRESHOLD_USB_LSB
#include "utils.h"

void drawMainMenu(void);
MenuReturn_e runMainMenu(const ButtonPress_e tuner_button,
                         const ButtonPress_e touch_button,
                         const Point touch_point,
                         const int16_t knob);
Menu_t mainMenu = {
  drawMainMenu,
  runMainMenu,
  nullptr
};

Menu_t* const rootMenu = &mainMenu;

bool mainMenuSelecting = false;//Tracks if we're selecting buttons with knob, or adjusting frequency
int16_t mainMenuSelectedItemRaw = 0;//Allow negative only for easier checks on wrap around

void drawMainMenu(void)
{
  displayClear(COLOR_BACKGROUND);
  invalidateVfo();
  for(uint8_t i = 0; i < MAIN_MENU_NUM_BUTTONS; ++i){
    drawButton(mainMenuButtons[i]);
  }
  drawVfo();
  drawVersion();
  drawCallsign();

  ltoa(GetActiveVfoFreq(),b,10);
  morseText(b);
}

void drawMainMenuIncrement()
{
  //State variables
  static uint32_t last_freq = 0;
  static Vfo_e last_vfo = Vfo_e::VFO_A;
  static VfoMode_e last_mode = VfoMode_e::VFO_MODE_LSB;
  static bool last_split = false;
  static bool last_rit = false;
  static TuningMode_e last_tuning = TuningMode_e::TUNE_SSB;

  if ((last_freq != GetActiveVfoFreq()) || (last_vfo != globalSettings.activeVfo))
  {
    drawVfo();
    drawBand();
    last_freq = GetActiveVfoFreq();
    last_vfo = globalSettings.activeVfo;

    // We set this here so that we're always hearing what's displayed
    setFrequency(last_freq);
  }

  if (last_mode != GetActiveVfoMode())
  {
    updateSidebandButton();
    last_mode = GetActiveVfoMode();
  }

  if(last_split != globalSettings.splitOn){
    drawVfo();
    drawButton(&bSpl);
    last_split = globalSettings.splitOn;
  }

  if(last_rit != globalSettings.ritOn){
    drawButton(&bRit);
    last_rit = globalSettings.ritOn;
  }

  if(last_tuning != globalSettings.tuningMode){
    drawButton(&bCw);
    last_tuning = globalSettings.tuningMode;
  }
}

void mainMenuTune(int16_t knob)
{
  if(0 == knob){
    //Nothing to do - we're already set!
    return;
  }

  const uint32_t current_freq = GetActiveVfoFreq();
  const uint32_t new_freq = current_freq + (50 * knob);
  
  SetActiveVfoFreq(new_freq);
  autoSelectSidebandChanged(current_freq);
}

MenuReturn_e runMainMenu(const ButtonPress_e tuner_button,
                         const ButtonPress_e touch_button,
                         const Point touch_point,
                         const int16_t knob)
{
  //Check tuner_button
  if(ButtonPress_e::NotPressed != tuner_button){
    switch(tuner_button){
      default://Fallthrough intended
      case ButtonPress_e::NotPressed:
      {
        //Nothing to do
        break;
      }
      case ButtonPress_e::ShortPress:
      {
        if(mainMenuSelecting){
          uint8_t menu_index = mainMenuSelectedItemRaw/MENU_KNOB_COUNTS_PER_ITEM;
          const Button* button = mainMenuButtons[menu_index];
          endSelector(button);
          button->select();
        }
        else{
          initSelector(&mainMenuSelectedItemRaw,
                       mainMenuButtons,
                       MAIN_MENU_NUM_BUTTONS,
                       MorsePlaybackType_e::PlayChar);
        }
        mainMenuSelecting = !mainMenuSelecting;

        //Don't handle touch or knob on this run
        return MenuReturn_e::StillActive;//main menu always returns StillActive
        break;
      }
      case ButtonPress_e::LongPress:
      {
        if(!globalSettings.morseMenuOn){
            globalSettings.morseMenuOn = true;//set before playing
            morseLetter(2);
        }
        else{
            morseLetter(4);
            globalSettings.morseMenuOn = false;//unset after playing
        }
        SaveSettingsToEeprom();
        //Don't handle touch or knob on this run
        return MenuReturn_e::StillActive;//main menu always returns StillActive
        break;
      }
    }//switch
  }//tuner_button

  else if(ButtonPress_e::NotPressed != touch_button){
    //We treat long and short presses the same, so no need to have a switch
    const Button* button = findPressedButton(mainMenuButtons,MAIN_MENU_NUM_BUTTONS,touch_point);
    if(nullptr != button){
      button->select();
    }
    else{
      //Touch detected, but not on our buttons, so ignore
    }
  }//touch_button

  else{//Neither button input type found, so handle the knob
    if(mainMenuSelecting){
      adjustSelector(&mainMenuSelectedItemRaw,
                     knob,
                     mainMenuButtons,
                     MAIN_MENU_NUM_BUTTONS,
                     MorsePlaybackType_e::PlayChar);
    }
    else{
      mainMenuTune(knob);
    }
  }

  drawMainMenuIncrement();

  return MenuReturn_e::StillActive;//main menu always returns StillActive
}