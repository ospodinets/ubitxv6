#include "menu_np_ql_shared.h"

#include "button.h"
#include "menu_utils.h"

MenuReturn_e runNpQlShared(const MenuInput& input,
                           int16_t *const menuSelectedItemRaw,
                           const Button *const *const menu_buttons,
                           const uint8_t menu_num_buttons,
                           ButtonPress_e *const selection_mode)
{
  switch(input.type){
    case MenuInputType::Tuner:
    {
      uint8_t menu_index = *menuSelectedItemRaw/MENU_KNOB_COUNTS_PER_ITEM;
      const Button* button = menu_buttons[menu_index];
      *selection_mode = input.press;
      button->select();
      break;
    }

    case MenuInputType::Touch:
    {
      const Button* button = findPressedButton(menu_buttons,menu_num_buttons,input.point);
      if(nullptr != button){
        *selection_mode = input.press;
        button->select();
      }
      else{
        //Touch detected, but not on our buttons, so ignore
      }
      break;
    }

    case MenuInputType::Knob:
    {
      adjustSelector(menuSelectedItemRaw,
                     input.knob,
                     menu_buttons,
                     menu_num_buttons);
      break;
    }

    case MenuInputType::Tick:
    {
      //Nothing changes on its own in these menus
      break;
    }
  }

  if(ButtonPress_e::NotPressed == *selection_mode){
    return MenuReturn_e::ExitedRedraw;
  }
  else{
    return MenuReturn_e::StillActive;
  }
}
