#include "menu_quicklist.h"
#include "menu_quicklist_buttons.h"

#include <avr/pgmspace.h>
#include <WString.h>//F()

#include "color_theme.h"
#include "menu_np_ql_shared.h"
#include "menu_utils.h"
#include "nano_gui.h"
#include "menu.h"

void initQuickList(void);
MenuReturn_e runQuickList(const MenuInput& input);
Menu_t quickList_menu = {
  initQuickList,
  runQuickList,
  nullptr
};

Menu_t *const quickListMenu = &quickList_menu;

int16_t quickListMenuSelectedItemRaw = 0;//Allow negative only for easier checks on wrap around

void drawQuickList(void)
{
  displayFillrect(0,LAYOUT_PAGE_Y,320,200,COLOR_BACKGROUND);
  for(uint8_t i = 0; i < QUICKLIST_MENU_NUM_BUTTONS; ++i){
    quickListMenuButtons[i]->draw();
  }
}

void initQuickList(void)
{
  quickListSelectionMode = ButtonPress_e::ShortPress;//Anything except NotPressed
  drawQuickList();
  initSelector(&quickListMenuSelectedItemRaw,
                quickListMenuButtons,
                QUICKLIST_MENU_NUM_BUTTONS);
}

MenuReturn_e runQuickList(const MenuInput& input)
{
  return runNpQlShared(input,
                       &quickListMenuSelectedItemRaw,
                       quickListMenuButtons,
                       QUICKLIST_MENU_NUM_BUTTONS,
                       &quickListSelectionMode);
}