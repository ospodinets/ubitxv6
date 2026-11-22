#include "menu_main.h"
#include "menu_main_buttons.h"

#include <Arduino.h>

#include "button.h"
#include "color_theme.h"
#include "menu_utils.h"
#include "nano_gui.h"
#include "settings.h"
#include "tuner.h"//THRESHOLD_USB_LSB
#include "tx_manager.h"
#include "utils.h"
#include "smeter.h"


void drawMainMenu(void);
MenuReturn_e runMainMenu(const MenuInput& input);
Menu_t mainMenu = {
  drawMainMenu,
  runMainMenu,
  nullptr
};

Menu_t* const rootMenu = &mainMenu;

bool mainMenuSelecting = false;//Tracks if we're selecting buttons with knob, or adjusting frequency
int16_t mainMenuSelectedItemRaw = 0;//Allow negative only for easier checks on wrap around
static constexpr int32_t MAIN_TUNING_STEP_HZ = 50;

void drawMainMenu(void)
{
  displayClear(COLOR_BACKGROUND);
  drawCallsign();
  drawVBat();
  invalidateVfo();
  drawVfo();
  for(uint8_t i = 0; i < MAIN_MENU_NUM_BUTTONS; ++i){
    mainMenuButtons[i]->draw();
  }
  drawBand();
  drawSMeter();
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

    // We set this here so that we're always hearing what's displayed.
    // Not while transmitting, when the oscillators are configured for TX.
    if(!txIsActive()){
      setFrequency(last_freq);
    }
  }

  if (last_mode != GetActiveVfoMode())
  {
    updateSidebandButton();
    last_mode = GetActiveVfoMode();
  }

  if(last_split != globalSettings.splitOn){
    drawVfo();
    updateSplitButton();
    last_split = globalSettings.splitOn;
  }

  if(last_rit != globalSettings.ritOn){
    updateRitButton();
    last_rit = globalSettings.ritOn;
  }

  if(last_tuning != globalSettings.tuningMode){
    updateCWButton();
    last_tuning = globalSettings.tuningMode;
  }


  updateSMeter();
  drawSMeterIncremental();
}

void mainMenuTune(int16_t knob)
{
  if(0 == knob){
    //Nothing to do - we're already set!
    return;
  }

  const uint32_t current_freq = GetActiveVfoFreq();
  const uint32_t new_freq = current_freq + (MAIN_TUNING_STEP_HZ * knob);
  
  SetActiveVfoFreq(new_freq);
  autoSelectSidebandChanged(current_freq);
}

MenuReturn_e runMainMenu(const MenuInput& input)
{
  switch(input.type){
    case MenuInputType::Tuner:
    {
      switch(input.press){
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
                         MAIN_MENU_NUM_BUTTONS);
          }
          mainMenuSelecting = !mainMenuSelecting;

          return MenuReturn_e::StillActive;//main menu always returns StillActive
        }
        case ButtonPress_e::LongPress:
        {
          SaveSettingsToEeprom();
          return MenuReturn_e::StillActive;//main menu always returns StillActive
        }
      }//switch
      break;
    }

    case MenuInputType::Touch:
    {
      //We treat long and short presses the same, so no need to have a switch
      const Button* button = findPressedButton(mainMenuButtons,MAIN_MENU_NUM_BUTTONS,input.point);
      if(nullptr != button){
        button->select();
      }
      else{
        //Touch detected, but not on our buttons, so ignore
      }
      break;
    }

    case MenuInputType::Knob:
    {
      if(mainMenuSelecting){
        adjustSelector(&mainMenuSelectedItemRaw,
                       input.knob,
                       mainMenuButtons,
                       MAIN_MENU_NUM_BUTTONS);
      }
      else{
        mainMenuTune(input.knob);
      }
      break;
    }

    case MenuInputType::Tick:
    {
      //Nothing to handle; the incremental draw below keeps the screen current
      break;
    }
  }//switch

  drawMainMenuIncrement();

  return MenuReturn_e::StillActive;//main menu always returns StillActive
}