#include "menu_quicklist_buttons.h"

#include <WString.h>//F()

#include "color_theme.h"
#include "nano_gui.h"
#include "scratch_space.h"
#include "settings.h"
#include "utils.h"
#include "menu.h"

static const int16_t LAYOUT_BUTTON_X = 3;
static const int16_t LAYOUT_BUTTON_Y = LAYOUT_PAGE_Y;
static const int16_t LAYOUT_BUTTON_WIDTH = 156;
static const int16_t LAYOUT_BUTTON_HEIGHT = 36;
static const int16_t LAYOUT_BUTTON_PITCH_X = LAYOUT_BUTTON_WIDTH + 1;
static const int16_t LAYOUT_BUTTON_PITCH_Y = 40;

ButtonPress_e quickListSelectionMode;

#define D_STRINGIFY(x) #x
#define D_STRING(x) D_STRINGIFY(x)
#define QUICKLIST_BUTTON_GENERATE(idX, idY, number) \
          void toQL##number(char* text_out, const uint16_t max_text_size);\
          void osQL##number();\
          static Button bQL##number = {\
            LAYOUT_BUTTON_X + idX*LAYOUT_BUTTON_PITCH_X,\
            static_cast<int16_t>(LAYOUT_BUTTON_Y + idY*LAYOUT_BUTTON_PITCH_Y),\
            LAYOUT_BUTTON_WIDTH,\
            LAYOUT_BUTTON_HEIGHT,\
            nullptr,\
            toQL##number,\
            bsQuickList,\
            osQL##number,\
            '0'+number\
          }

ButtonStatus_e bsQuickList(void){
  return ButtonStatus_e::Stateless;
}

//Unfortunately there's no easy way to auto-generate NUM_QUICKLIST_SETTINGS copies of this
QUICKLIST_BUTTON_GENERATE(0, 0, 0);
QUICKLIST_BUTTON_GENERATE(1, 0, 1);
QUICKLIST_BUTTON_GENERATE(0, 1, 2);
QUICKLIST_BUTTON_GENERATE(1, 1, 3);
QUICKLIST_BUTTON_GENERATE(0, 2, 4);
QUICKLIST_BUTTON_GENERATE(1, 2, 5);

void osQLCancel();
static Button bQLCancel = {
  3,
  static_cast<int16_t>(LAYOUT_BUTTON_Y + 3 * LAYOUT_BUTTON_PITCH_Y),
  320 - 6,
  LAYOUT_BUTTON_HEIGHT,
  "Cancel",
  nullptr,
  bsQuickList,
  osQLCancel,
  'C'
};

//Declare in menu select order, not graphical order
const Button* const quickListMenuButtons [] = {
  &bQL0, &bQL1, &bQL2, &bQL3, &bQL4, &bQL5, &bQLCancel
};
const uint8_t QUICKLIST_MENU_NUM_BUTTONS = sizeof(quickListMenuButtons)/sizeof(quickListMenuButtons[0]);

void osQuickListRecall(uint8_t index)
{
  SetActiveVfoFreq(globalSettings.quickList[index].frequency);
  SetActiveVfoMode(globalSettings.quickList[index].mode);
}

void osQuickListSave(uint8_t index)
{
  globalSettings.quickList[index].frequency = GetActiveVfoFreq();
  globalSettings.quickList[index].mode = GetActiveVfoMode();
  SaveSettingsToEeprom();
}

void osQuickList(uint8_t index)
{
  if(ButtonPress_e::ShortPress == quickListSelectionMode){
    osQuickListRecall(index);
  }
  else if(ButtonPress_e::LongPress == quickListSelectionMode){
    osQuickListSave(index);
  }
  quickListSelectionMode = ButtonPress_e::NotPressed;//Selection was made. Exit.
}

#define QUICKLIST_ONSELECT_GENERATE(number) \
          void osQL##number(void){\
            osQuickList(number);\
          }

//Unfortunately there's no easy way to auto-generate NUM_QUICKLIST_SETTINGS copies of this
QUICKLIST_ONSELECT_GENERATE(0);
QUICKLIST_ONSELECT_GENERATE(1);
QUICKLIST_ONSELECT_GENERATE(2);
QUICKLIST_ONSELECT_GENERATE(3);
QUICKLIST_ONSELECT_GENERATE(4);
QUICKLIST_ONSELECT_GENERATE(5);

#define QUICKLIST_TEXTOVERRIDE_GENERATE(number) \
          void toQL##number(char* text_out, const uint16_t max_text_size){\
            toQuickList(number, text_out, max_text_size);\
          }

void toQuickList(uint8_t index, char* text_out, const uint16_t max_text_size)
{
  if(max_text_size < 2){
    return;//Can't do much with that space
  }
  if(max_text_size < (3+10+1)){
    //Give an indicator that's debuggable
    text_out[0] = 'X';
    text_out[1] = '\0';
    return;
  }

  //Normal operation
  text_out[0] = '0'+index+1;//Assume a 1-digit number for now
  text_out[1] = ':';
  text_out[2] = ' ';
  formatFreq(globalSettings.quickList[index].frequency, text_out+3, max_text_size-3, 10);
}

//Unfortunately there's no easy way to auto-generate NUM_QUICKLIST_SETTINGS copies of this
QUICKLIST_TEXTOVERRIDE_GENERATE(0);
QUICKLIST_TEXTOVERRIDE_GENERATE(1);
QUICKLIST_TEXTOVERRIDE_GENERATE(2);
QUICKLIST_TEXTOVERRIDE_GENERATE(3);
QUICKLIST_TEXTOVERRIDE_GENERATE(4);
QUICKLIST_TEXTOVERRIDE_GENERATE(5);

void osQLCancel()
{
  quickListSelectionMode = ButtonPress_e::NotPressed;
}
  