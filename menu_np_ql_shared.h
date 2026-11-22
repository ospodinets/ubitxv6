#pragma once

#include "button.h"
#include "menu.h"

MenuReturn_e runNpQlShared(const MenuInput& input,
                           int16_t *const menuSelectedItemRaw,
                           const Button *const *const menu_buttons,
                           const uint8_t menu_num_buttons,
                           ButtonPress_e *const selection_mode);

