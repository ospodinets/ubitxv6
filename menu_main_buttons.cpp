#include "menu_main_buttons.h"

#include <avr/pgmspace.h>
#include <string.h>
#include <WString.h>//F()
#include <Arduino.h>

#include "bands.h"
#include "button.h"
#include "callsign.h"
#include "color_theme.h"
#include "label.h"
#include "menu.h"
#include "menu_main.h"
#include "menu_numpad.h"
#include "menu_quicklist.h"
#include "morse.h"
#include "nano_gui.h"
#include "scratch_space.h"
#include "settings.h"
#include "setup.h"
#include "tuner.h"
#include "utils.h"
#include "version.h"

static const unsigned int DISPLAY_WIDTH = 320;
static const unsigned int DISPLAY_HEIGHT = 240;

static const unsigned int LAYOUT_CALLSIGN_TEXT_X = 3;
static const unsigned int LAYOUT_CALLSIGN_TEXT_Y = 3;
static const unsigned int LAYOUT_CALLSIGN_TEXT_WIDTH = 160;
static const unsigned int LAYOUT_CALLSIGN_TEXT_HEIGHT = 30;

static const unsigned int LAYOUT_VBAT_TEXT_X = 160;
static const unsigned int LAYOUT_VBAT_TEXT_Y = 3;
static const unsigned int LAYOUT_VBAT_TEXT_WIDTH = 160;
static const unsigned int LAYOUT_VBAT_TEXT_HEIGHT = 30;

static const unsigned int LAYOUT_TX_X = 3;
static const unsigned int LAYOUT_TX_Y = LAYOUT_VBAT_TEXT_Y + LAYOUT_VBAT_TEXT_HEIGHT + 6;
static const unsigned int LAYOUT_TX_WIDTH = 48;
static const unsigned int LAYOUT_TX_HEIGHT = 32;

static const int16_t LAYOUT_VFO_MAIN_X = LAYOUT_TX_X + LAYOUT_TX_WIDTH + 3;
static const int16_t LAYOUT_VFO_MAIN_Y = LAYOUT_TX_Y - 6;
static const int16_t LAYOUT_VFO_MAIN_WIDTH = 230;
static const int16_t LAYOUT_VFO_MAIN_HEIGHT = 48;

static const unsigned int LAYOUT_SSB_X = LAYOUT_VFO_MAIN_X + LAYOUT_VFO_MAIN_WIDTH + 6;
static const unsigned int LAYOUT_SSB_Y = LAYOUT_VFO_MAIN_Y;
static const unsigned int LAYOUT_SSB_WIDTH = 30;
static const unsigned int LAYOUT_SSB_HEIGHT = 23;

static const unsigned int LAYOUT_BAND_X = LAYOUT_SSB_X;
static const unsigned int LAYOUT_BAND_Y = LAYOUT_SSB_Y + LAYOUT_SSB_HEIGHT + 3;
static const unsigned int LAYOUT_BAND_WIDTH = 30;
static const unsigned int LAYOUT_BAND_HEIGHT = 23;

static const unsigned int LAYOUT_VFO_SEC_X = LAYOUT_VFO_MAIN_X;
static const unsigned int LAYOUT_VFO_SEC_Y = LAYOUT_VFO_MAIN_Y + LAYOUT_VFO_MAIN_HEIGHT + 3;
static const unsigned int LAYOUT_VFO_SEC_WIDTH = 230;
static const unsigned int LAYOUT_VFO_SEC_HEIGHT = 36;

// Buttons
static const unsigned int LAYOUT_BUTTON_X = 0;
static const unsigned int LAYOUT_BUTTON_Y = LAYOUT_VFO_SEC_Y + LAYOUT_VFO_SEC_HEIGHT + 15;
static const unsigned int LAYOUT_BUTTON_WIDTH = 60;
static const unsigned int LAYOUT_BUTTON_HEIGHT = 36;
static const unsigned int LAYOUT_BUTTON_HSPACING = 3;
static const unsigned int LAYOUT_BUTTON_VSPACING = 3;
static const unsigned int LAYOUT_BUTTON_HPITCH = LAYOUT_BUTTON_WIDTH + LAYOUT_BUTTON_HSPACING;
static const unsigned int LAYOUT_BUTTON_VPITCH = LAYOUT_BUTTON_HEIGHT + LAYOUT_BUTTON_VSPACING;


static const unsigned int LAYOUT_VERSION_TEXT_X = DISPLAY_WIDTH - DISPLAY_WIDTH/2;
static const unsigned int LAYOUT_VERSION_TEXT_Y = DISPLAY_HEIGHT - 6;
static const unsigned int LAYOUT_VERSION_TEXT_WIDTH = DISPLAY_WIDTH/2;
static const unsigned int LAYOUT_VERSION_TEXT_HEIGHT = 6;

int16_t LAYOUT_PAGE_Y = LAYOUT_VFO_SEC_Y;

void drawTx()
{
  if(globalSettings.txActive){
    strncpy_P(b,(const char*)F("TX"),sizeof(b));

    Style style;
    style.textColor = DISPLAY_WHITE;
    style.backgroundColor = DISPLAY_RED;
    style.borderColor = DISPLAY_WHITE;
    style.justification = TextJustification_e::Center;
    style.fontFamily = FontFamily_e::Regular;
    style.fontSize = 1;
    displayText(b,LAYOUT_TX_X,LAYOUT_TX_Y,LAYOUT_TX_WIDTH,LAYOUT_TX_HEIGHT,style);
  }
  else{
    displayFillrect(LAYOUT_TX_X,LAYOUT_TX_Y,LAYOUT_TX_WIDTH,LAYOUT_TX_HEIGHT,COLOR_BACKGROUND);
  }
}

void drawVersion()
{
  strncpy_P(b, VERSION_STRING, sizeof(b));
  Style style;
  style.textColor = COLOR_VERSION_TEXT;
  style.backgroundColor = COLOR_BACKGROUND;
  style.borderColor = COLOR_BACKGROUND;
  style.justification = TextJustification_e::Right;
  style.fontFamily = FontFamily_e::SmallTitles;
  style.fontSize = 1;
  displayText(b, LAYOUT_VERSION_TEXT_X, LAYOUT_VERSION_TEXT_Y, LAYOUT_VERSION_TEXT_WIDTH, LAYOUT_VERSION_TEXT_HEIGHT, style);
}

void drawCallsign()
{
  strncpy_P(b, CALLSIGN_STRING, sizeof(b));
  Style style;
  style.textColor = COLOR_VERSION_TEXT;
  style.backgroundColor = COLOR_BACKGROUND;
  style.borderColor = COLOR_BACKGROUND;
  style.justification = TextJustification_e::Left;
  style.fontFamily = FontFamily_e::Regular;
  style.fontSize = 1;
  displayText(b, LAYOUT_CALLSIGN_TEXT_X, LAYOUT_CALLSIGN_TEXT_Y, LAYOUT_CALLSIGN_TEXT_WIDTH, LAYOUT_CALLSIGN_TEXT_HEIGHT, style);
}

Style styleVfoMain = {
    DISPLAY_WHITE,
    COLOR_BACKGROUND,
    COLOR_BACKGROUND,
    TextJustification_e::Right,
    FontFamily_e::BigDigits,
    1
  };

void toVfoMain(char* text_out, const uint16_t max_text_size);

Label lVfoMain(
  LAYOUT_VFO_MAIN_X, 
  LAYOUT_VFO_MAIN_Y,
  LAYOUT_VFO_MAIN_WIDTH,
  LAYOUT_VFO_MAIN_HEIGHT,
  nullptr,
  &toVfoMain,
  &styleVfoMain
);

void drawVfo()
{
  lVfoMain.draw();
}

void invalidateVfo()
{
  lVfoMain.invalidate();
}

Style styleBand = {
    COLOR_INACTIVE_TEXT,
    COLOR_INACTIVE_BACKGROUND,
    COLOR_INACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::SmallTitles,
    1
  };

void toBand(char* text_out, const uint16_t max_text_size);
Label lBand(
  LAYOUT_BAND_X,
  LAYOUT_BAND_Y,
  LAYOUT_BAND_WIDTH,
  LAYOUT_BAND_HEIGHT,
  nullptr,
  toBand,
  &styleBand
);

void drawBand()
{
  lBand.draw();
}

Style styleSSBactive = {
    COLOR_ACTIVE_TEXT,
    COLOR_ACTIVE_BACKGROUND,
    COLOR_ACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Icons,
    1
  };

Style styleSSBInactive = {
    COLOR_INACTIVE_TEXT,
    COLOR_INACTIVE_BACKGROUND,
    COLOR_INACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Icons,
    1
  };

ButtonStatus_e bsIgnore();
void toSSB(char* text_out, const uint16_t max_text_size);
ButtonStatus_e bsSSB();
void osSSB();
static Button bSSB = {
  LAYOUT_SSB_X,
  LAYOUT_SSB_Y,
  LAYOUT_SSB_WIDTH,
  LAYOUT_SSB_HEIGHT,
  nullptr,
  toSSB,
  bsIgnore,
  osSSB,
  'S',
  &styleSSBInactive,
  &styleSSBactive
};

Style styleVfoSec = {
    DISPLAY_WHITE,
    COLOR_BACKGROUND,
    COLOR_BACKGROUND,
    TextJustification_e::Right,
    FontFamily_e::SmallDigits,
    1
  };

void toVfoSec(char* text_out, const uint16_t max_text_size);
ButtonStatus_e bsVfoSec();
void osVfoSec();
static Button bVfoSec = {
  LAYOUT_VFO_SEC_X,
  LAYOUT_VFO_SEC_Y,
  LAYOUT_VFO_SEC_WIDTH,
  LAYOUT_VFO_SEC_HEIGHT,
  nullptr,
  toVfoSec,
  bsVfoSec,
  osVfoSec,
  'B',
  &styleVfoSec,
  &styleVfoSec
};

constexpr char txtSpl [] PROGMEM = "SPL";
ButtonStatus_e bsSpl();
void osSpl();
Button bSpl = {
  LAYOUT_BUTTON_X,
  LAYOUT_BUTTON_Y,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtSpl,
  nullptr,
  bsSpl,
  osSpl,
  'S'
};

constexpr char txtRit [] PROGMEM = "RIT";
ButtonStatus_e bsRit();
void osRit();
Button bRit = {
  LAYOUT_BUTTON_X + LAYOUT_BUTTON_HPITCH,
  LAYOUT_BUTTON_Y + 0*LAYOUT_BUTTON_VPITCH,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtRit,
  nullptr,
  bsRit,
  osRit,
  'R'
};

constexpr char txtCw [] PROGMEM = "CW";
ButtonStatus_e bsCw();
void osCw();
Button bCw = {
  LAYOUT_BUTTON_X + 2*LAYOUT_BUTTON_HPITCH,
  LAYOUT_BUTTON_Y + 0*LAYOUT_BUTTON_VPITCH,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtCw,
  nullptr,
  bsCw,
  osCw,
  'C'
};

constexpr char txtUp [] PROGMEM = "UP";
ButtonStatus_e bsUp();
void osUp();
static Button bUp = {
  LAYOUT_BUTTON_X + 3*LAYOUT_BUTTON_HPITCH,
  LAYOUT_BUTTON_Y + 0*LAYOUT_BUTTON_VPITCH,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtUp,
  nullptr,
  bsIgnore,
  osUp,
  'U'
};

constexpr char txtDown [] PROGMEM = "DN";
ButtonStatus_e bsDown();
void osDown();
static Button bDown = {
  LAYOUT_BUTTON_X + 4*LAYOUT_BUTTON_HPITCH,
  LAYOUT_BUTTON_Y + 0*LAYOUT_BUTTON_VPITCH,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtDown,
  nullptr,
  bsIgnore,
  osDown,
  'D'
};

Style styleIconicActive = {
    COLOR_ACTIVE_TEXT,
    COLOR_ACTIVE_BACKGROUND,
    COLOR_ACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Icons,
    1
  };

Style styleIconicInactive = {
    COLOR_INACTIVE_TEXT,
    COLOR_INACTIVE_BACKGROUND,
    COLOR_INACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Icons,
    1
  };

constexpr char txtQuickList [] PROGMEM = "\x01";//star icon
void osQuickList();
static Button bQuickList = {
  LAYOUT_BUTTON_X + 2*LAYOUT_BUTTON_HPITCH,
  LAYOUT_BUTTON_Y + 1*LAYOUT_BUTTON_VPITCH,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtQuickList,
  nullptr,
  bsIgnore,
  osQuickList,
  'Q',
  &styleIconicInactive,
  &styleIconicActive
};

constexpr char txtMenu [] PROGMEM = "\x02";//options icon
ButtonStatus_e bsIgnore();
void osMenu();
static Button bMenu = {
  LAYOUT_BUTTON_X + 3*LAYOUT_BUTTON_HPITCH,
  LAYOUT_BUTTON_Y + 1*LAYOUT_BUTTON_VPITCH,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtMenu,
  nullptr,
  bsIgnore,
  osMenu,
  'M',
  &styleIconicInactive,
  &styleIconicActive
};

constexpr char txtNumpad [] PROGMEM = "\x03";//numpad icon
ButtonStatus_e bsIgnore();
void osNumpad();
static Button bNumpad = {
  LAYOUT_BUTTON_X + 4*LAYOUT_BUTTON_HPITCH,
  LAYOUT_BUTTON_Y + 1*LAYOUT_BUTTON_VPITCH,
  LAYOUT_BUTTON_WIDTH,
  LAYOUT_BUTTON_HEIGHT,
  txtNumpad,
  nullptr,
  bsIgnore,
  osNumpad,
  'F',
  &styleIconicInactive,
  &styleIconicActive
};

const Button* const mainMenuButtons [] = {
    &bSSB,
    &bVfoSec,
    &bSpl, &bRit, &bCw, &bUp, &bDown,
    &bQuickList,  &bMenu, &bNumpad
};

const uint8_t MAIN_MENU_NUM_BUTTONS = sizeof(mainMenuButtons) / sizeof(mainMenuButtons[0]);

void toVfoSec(char *text_out, const uint16_t max_text_size)
{
  if (max_text_size < 2)
  {
    return; // Can't do much with that space
  }

  if (max_text_size < (3 + 10 + 1))
  {
    // Give an indicator that's debuggable
    text_out[0] = 'X';
    text_out[1] = '\0';
    return;
  }

  // Normal operation
  if (globalSettings.splitOn || globalSettings.ritOn)
  {
    text_out[0] = ';';
    text_out[1] = ' ';
  }
  else
  {
    text_out[0] = ' ';
    text_out[1] = ' ';
  }

  // if rit is active the rit frequesncy is a transmit frequency
  if (globalSettings.ritOn)
  {
    formatFreq(globalSettings.ritFrequency, text_out + 2, max_text_size - 3, 10);
  }
  else if (globalSettings.activeVfo == VFO_A)
  {
    formatFreq(globalSettings.vfoB.frequency, text_out + 2, max_text_size - 3, 10);
  }
  else if (globalSettings.activeVfo == VFO_B)
  {
    formatFreq(globalSettings.vfoA.frequency, text_out + 2, max_text_size - 3, 10);
  }
  else
  {
    text_out[3] = '?';
    text_out[4] = '\0';
  }
  for (char *c = text_out; *c; ++c)
  {
    if ('.' == *c)
    {
      *c = ':';
    }
  }
}

ButtonStatus_e bsVfoSec(){
  return ButtonStatus_e::Inactive;
}

void osVfoSec()
{
  if (globalSettings.ritOn)
  {
    osRit(); // Turn off RIT
  }

  globalSettings.activeVfo = (globalSettings.activeVfo == VFO_A) ? VFO_B : VFO_A;
  SaveSettingsToEeprom();

  ltoa(GetActiveVfoFreq(), b, 10);
  morseText(b);

  bVfoSec.draw();
  drawVfo();
  drawBand();
  updateSidebandButton();
}

ButtonStatus_e bsRit(){
  return globalSettings.ritOn ? ButtonStatus_e::Active : ButtonStatus_e::Inactive;
}

void osRit()
{
  if (!globalSettings.ritOn)
  {
    globalSettings.ritOn = true;
    globalSettings.ritFrequency = GetActiveVfoFreq();
  }
  else
  {
    globalSettings.ritOn = false;
    setFrequency(globalSettings.ritFrequency);
  }

  bVfoSec.draw();
  bRit.draw();
}

void updateSidebandButton()
{
  bSSB.draw();
}

void toVfoMain(char* text_out, const uint16_t max_text_size)
{
  if (max_text_size < 2)
  {
    return; // Can't do much with that space
  }

  if (max_text_size < (3 + 10 + 1))
  {
    // Give an indicator that's debuggable
    text_out[0] = 'X';
    text_out[1] = '\0';
    return;
  }

  formatFreq(GetActiveVfoFreq(), text_out, max_text_size);
  for (char *c = text_out; *c; ++c)
  {
    if ('.' == *c)
    {
      *c = ':';
    }
  }
}

void toBand(char* text_out, const uint16_t max_text_size)
{
  if (max_text_size < 2)
  {
    return; // Can't do much with that space
  }

  if (max_text_size < (3 + 10 + 1))
  {
    // Give an indicator that's debuggable
    text_out[0] = 'X';
    text_out[1] = '\0';
    return;
  }

  getBandString(GetActiveVfoFreq(), text_out, max_text_size);
}

void toSSB(char* text_out, const uint16_t max_text_size)
{
  if (max_text_size < 2)
  {
    return; // Can't do much with that space
  }

  if (VFO_MODE_USB == GetActiveVfoMode())
  {
    text_out[0] = ICON_USB;
    text_out[1] = '\0';
  }
  else if (VFO_MODE_LSB == GetActiveVfoMode())
  {
    text_out[0] = ICON_LSB;
    text_out[1] = '\0';
  }
}

void osSSB()
{
  auto mode = GetActiveVfoMode() == VFO_MODE_LSB ? VFO_MODE_USB : VFO_MODE_LSB;
  SetActiveVfoMode(mode);
  setFrequency(GetActiveVfoFreq());
  SaveSettingsToEeprom();

  bSSB.draw();
}

ButtonStatus_e bsCw()
{
  return (TuningMode_e::TUNE_CW == globalSettings.tuningMode) ? ButtonStatus_e::Active : ButtonStatus_e::Inactive;
}

void osCw()
{
  if (TuningMode_e::TUNE_CW != globalSettings.tuningMode)
  {
    globalSettings.tuningMode = TuningMode_e::TUNE_CW;
  }
  else
  {
    globalSettings.tuningMode = TuningMode_e::TUNE_SSB;
  }

  setFrequency(GetActiveVfoFreq());

  bCw.draw();
}

ButtonStatus_e bsSpl()
{
  return globalSettings.splitOn ? ButtonStatus_e::Active : ButtonStatus_e::Inactive;
}

void osSpl()
{
  globalSettings.splitOn = !globalSettings.splitOn;

  bSpl.draw();
  bVfoSec.draw();
}

void osUp()
{
  const uint32_t old_freq = GetActiveVfoFreq();
  SetActiveVfoFreq(getNextBandFreq(old_freq, true));
  if (autoSelectSidebandChanged(old_freq))
  {
    updateSidebandButton();
  }
  drawVfo();
  drawBand();
}

void osDown()
{
  const uint32_t old_freq = GetActiveVfoFreq();
  SetActiveVfoFreq(getNextBandFreq(old_freq, false));
  if (autoSelectSidebandChanged(old_freq))
  {
    updateSidebandButton();
  }
  drawVfo();
  drawBand();
}

ButtonStatus_e bsIgnore(){
  return ButtonStatus_e::Stateless;
}

void osQuickList(){
  enterSubmenu(quickListMenu);
}

void osMenu(){
  enterSubmenu(setupMenu);
}

void osNumpad(){
  enterSubmenu(numpadMenu);
}
