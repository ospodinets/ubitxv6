#include "menu_utils.h"

#include "button.h"
#include "color_theme.h"
#include "nano_gui.h"
#include "utils.h"

const Button* findPressedButton(const Button* const* buttons,
                                const uint8_t num_buttons,
                                const Point touch_point)
{
  for(uint16_t i = 0; i < num_buttons; ++i){
    const Button* button = buttons[i];
    if((button->x() <= touch_point.x)
     &&(touch_point.x <= button->x() + button->width())
     &&(button->y() <= touch_point.y)
     &&(touch_point.y <= button->y() + button->height())){
       return button;
     }
  }

  return nullptr;
}

void movePuck(const Button *const b_old,
              const Button *const b_new)
{
  if(nullptr != b_old){
    displayRect(b_old->x(),b_old->y(),b_old->width(),b_old->height(),COLOR_INACTIVE_BORDER);
  }
  if(nullptr != b_new){
    displayRect(b_new->x(),b_new->y(),b_new->width(),b_new->height(),COLOR_ACTIVE_BORDER);
  }
}

void initSelector(int16_t *const raw_select_val_in_out,
                  const Button* const* buttons,
                  const uint8_t num_buttons)
{
  *raw_select_val_in_out = 0;
  if(0 < num_buttons){
    const Button* button = buttons[0];
    movePuck(nullptr,button);
  }
}

void adjustSelector(int16_t *const raw_select_val_in_out,
                    const int16_t knob,
                    const Button* const* buttons,
                    const uint8_t num_buttons)
{
  const uint8_t prev_select = (*raw_select_val_in_out)/MENU_KNOB_COUNTS_PER_ITEM;
  *raw_select_val_in_out = LIMIT((*raw_select_val_in_out)+knob,0,num_buttons*MENU_KNOB_COUNTS_PER_ITEM - 1);
  const uint8_t new_select = (*raw_select_val_in_out)/MENU_KNOB_COUNTS_PER_ITEM;
  if(prev_select != new_select){
    const Button* prev_button = buttons[prev_select];
    const Button* new_button = buttons[new_select];

    movePuck(prev_button,new_button);
  }
}

void endSelector(const Button *const button)
{
  movePuck(button,nullptr);
}
