#pragma once

#include <stdint.h>
#include "button_press_e.h"
#include "point.h"

enum MenuReturn_e : uint8_t {
  StillActive,
  ExitedRedraw,
  ExitedNoRedraw
};

//What a menu is being told. Menus never read the hardware themselves.
enum class MenuInputType : uint8_t {
  Tick,  //Nothing was input; a chance to refresh anything that changes on its own, e.g. the S-meter
  Tuner, //Tuner (encoder) button was pressed
  Touch, //Touch screen was pressed
  Knob   //Encoder was turned
};

struct MenuInput {
  MenuInputType type;
  ButtonPress_e press;//Tuner and Touch: short or long press
  Point point;//Touch: calibrated screen position
  int16_t knob;//Knob: number of encoder counts turned
};

struct Menu_t {
  void (*const initMenu)();//Any initial draw routines or state initialization
  MenuReturn_e (*const runMenu)(const MenuInput& input);
  Menu_t* active_submenu;
};

static const uint8_t MENU_KNOB_COUNTS_PER_ITEM = 10;

//Passes one input to the deepest active menu
void runActiveMenu(const MenuInput& input);

void enterSubmenu(Menu_t *const submenu);

extern int16_t LAYOUT_PAGE_Y;
