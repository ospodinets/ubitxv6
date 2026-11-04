#include <Arduino.h>
#include "nano_gui.h"
#include "colors.h"
#include "pin_definitions.h"
#include "push_button.h"
#include "scratch_space.h"
#include "settings.h"
#include "touch.h"
#include "nano_font.h"

#include <SPI.h>
#include <avr/pgmspace.h>


/*****************
 * Begin TFT functions
 *****************/
#include <Adafruit_ILI9341.h>

Adafruit_ILI9341 tft(PIN_TFT_CS, PIN_TFT_DC);

static const GFXfont *activeFont = nullptr;
static uint8_t activeTextSize = 1;

static void setDisplayFont(const GFXfont *font)
{
  activeFont = font;
  tft.setFont(font);
}

static void setDisplayTextSize(uint8_t size)
{
  activeTextSize = (size > 0) ? size : 1;
  tft.setTextSize(activeTextSize);
}

const GFXfont* getFont(FontFamily_e fontFamily)
{
  switch (fontFamily)
  {
    case FontFamily_e::Regular:
      return font_regular();
    case FontFamily_e::SmallTitles:
      return font_small_titles();
    case FontFamily_e::BigDigits:
      return font_big_digits();
    case FontFamily_e::SmallDigits:
      return font_small_digits();
    case FontFamily_e::Icons:
      return font_icons();
    default:
      return font_regular();
  }
}


void displayInit(void){
  //Pulling this low 6 times should exit deep sleep mode
  pinMode(PIN_TFT_CS,OUTPUT);
  for(uint8_t i = 0; i < 6; ++i){
    digitalWrite(PIN_TFT_CS,HIGH);
    digitalWrite(PIN_TFT_CS,LOW);
  }
  digitalWrite(PIN_TFT_CS,HIGH);//Disable writing for now

  tft.begin();
  setDisplayFont(font_regular());
  tft.setTextWrap(false);
  tft.setTextColor(DISPLAY_GREEN,DISPLAY_BLACK);
  setDisplayTextSize(1);
  tft.setRotation(1);
}

void displayPixel(unsigned int x, unsigned int y, unsigned int c){
  tft.fillRect(x,y,1,1,c);
}

void displayHline(unsigned int x, unsigned int y, unsigned int w, unsigned int c){
  tft.fillRect(x,y,w,1,c);
}

void displayVline(unsigned int x, unsigned int y, unsigned int l, unsigned int c){
  tft.fillRect(x,y,1,l,c);
}

void displayClear(unsigned int color){
  tft.fillRect(0,0,tft.width(),tft.height(),color);
}

void displayRect(unsigned int x,unsigned int y,unsigned int w,unsigned int h,unsigned int c){
  tft.fillRect(x,y,w,1,c);
  tft.fillRect(x,y,1,h,c);
  tft.fillRect(x,y+h-1,w,1,c);
  tft.fillRect(x+w-1,y,1,h,c);
}

void displayFillrect(unsigned int x,unsigned int y,unsigned int w,unsigned int h,unsigned int c){
  tft.fillRect(x,y,w,h,c);
}

void displayChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg) {
  tft.drawChar(x,y,c,color,bg,1);
}

void displayRawText(const char *text, int x1, int y1, int w, int color, int background){
  tft.setTextColor(color,background);
  const int right = x1 + w;
  int cursorX = x1;
  int cursorY = y1;
  bool lineOverflow = false;

  while (*text != '\0') {
    const unsigned char c = static_cast<unsigned char>(*text++);
    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      cursorX = x1;
      lineOverflow = false;
      if (activeFont != nullptr) {
        cursorY += activeTextSize * pgm_read_byte(&activeFont->yAdvance);
      } else {
        cursorY += activeTextSize * 8;
      }
      continue;
    }

    if (activeFont != nullptr) {
      const uint16_t first = pgm_read_word(&activeFont->first);
      const uint16_t last = pgm_read_word(&activeFont->last);
      if (c < first || c > last) {
        continue;
      }

      const GFXglyph *glyphs = static_cast<const GFXglyph *>(pgm_read_ptr(&activeFont->glyph));
      const GFXglyph *glyph = &glyphs[c - first];
      const uint8_t glyphWidth = pgm_read_byte(&glyph->width);
      const uint8_t glyphHeight = pgm_read_byte(&glyph->height);
      const int8_t xOffset = static_cast<int8_t>(pgm_read_byte(&glyph->xOffset));
      const int8_t yOffset = static_cast<int8_t>(pgm_read_byte(&glyph->yOffset));
      const int glyphLeft = cursorX + xOffset * activeTextSize;
      const int glyphRight = glyphLeft + glyphWidth * activeTextSize;
      const int glyphTop = cursorY + yOffset * activeTextSize;
      const int glyphBottom = glyphTop + glyphHeight * activeTextSize;

      if (!lineOverflow && glyphLeft >= x1 && glyphRight <= right &&
          glyphTop >= 0 && glyphBottom <= tft.height()) {
        tft.drawChar(cursorX, cursorY, c, color, background, activeTextSize);
      } else if (glyphRight > right) {
        lineOverflow = true;
      }

      cursorX += pgm_read_byte(&glyph->xAdvance) * activeTextSize;
    } else {
      const int glyphRight = cursorX + 6 * activeTextSize;
      if (!lineOverflow && cursorX >= x1 && glyphRight <= right &&
          cursorY >= 0 && cursorY + 8 * activeTextSize <= tft.height()) {
        tft.drawChar(cursorX, cursorY, c, color, background, activeTextSize);
      } else if (glyphRight > right) {
        lineOverflow = true;
      }
      cursorX = glyphRight;
    }
  }
}

void displayText(const char *const text, int x1, int y1, int w, int h, const Style& style)
{
  setDisplayFont(getFont(style.fontFamily));
  setDisplayTextSize(style.fontSize);
  displayText(text, x1, y1, w, h, style.textColor, style.backgroundColor, style.borderColor, style.justification);
}

// deprecated
void displayText(const char *const text, int x1, int y1, int w, int h, int color, int background, int border,
   TextJustification_e justification)
{
  displayFillrect(x1, y1, w, h, background);
  displayRect(x1, y1, w, h, border);

  int16_t x1_out;
  int16_t y1_out;
  uint16_t width_out;
  uint16_t height_out;
  tft.getTextBounds(text, x1, y1, &x1_out, &y1_out, &width_out, &height_out);
  if (TextJustification_e::Center == justification)
  {
    x1 += (w - ((int32_t)width_out + (x1_out - x1))) / 2;
  }
  else if (TextJustification_e::Right == justification)
  {
    x1 += w - ((int32_t)width_out + (x1_out - x1));
  }
  else
  {
    x1 += 2; // Give a little bit of padding from the border
  }

  const uint8_t yAdvance = (activeFont != nullptr)
    ? pgm_read_byte(&activeFont->yAdvance)
    : 8;
  y1 += (yAdvance + h - static_cast<int32_t>(height_out)) / 2;
  displayRawText(text, x1, y1, w, color, background);
}

void drawCross(int16_t x_center,int16_t y_center,uint16_t color)
{
  constexpr uint8_t HALF_SIZE = 10;
  displayHline(x_center-HALF_SIZE,y_center,2*HALF_SIZE,color);
  displayVline(x_center,y_center-HALF_SIZE,2*HALF_SIZE,color);
}

void setupTouch(){
  constexpr int16_t CROSS_CORNER_OFFSET = 20;
  const int16_t displayWidth = tft.width();
  const int16_t displayHeight = tft.height();
  const Point CROSS_CORNER_POINTS [] = {
    {CROSS_CORNER_OFFSET,CROSS_CORNER_OFFSET},//Top left
    {static_cast<int16_t>(displayWidth-CROSS_CORNER_OFFSET),CROSS_CORNER_OFFSET},//Top right
    {CROSS_CORNER_OFFSET, static_cast<int16_t>(displayHeight-CROSS_CORNER_OFFSET)},//Bottom left
    {static_cast<int16_t>(displayWidth-CROSS_CORNER_OFFSET),static_cast<int16_t>(displayHeight-CROSS_CORNER_OFFSET)}//Bottom right
  };
  
  displayClear(DISPLAY_BLACK);
  strncpy_P(b,(const char*)F("Click on the cross\nPush tune to cancel"),sizeof(b));
  displayText(b, 20,100, 200, 50, DISPLAY_WHITE, DISPLAY_BLACK, DISPLAY_BLACK);

  Point cal_points[sizeof(CROSS_CORNER_POINTS)/sizeof(CROSS_CORNER_POINTS[0])];

  for(uint8_t i = 0; i < sizeof(CROSS_CORNER_POINTS)/sizeof(CROSS_CORNER_POINTS[0]); ++i){
    drawCross(CROSS_CORNER_POINTS[i].x,CROSS_CORNER_POINTS[i].y,DISPLAY_WHITE);
    while(!readTouch(&cal_points[i])){
      if(ButtonPress_e::NotPressed != CheckTunerButton()){
        return;
      }
      delay(100);
    }
    while(readTouch(&cal_points[i])){
      delay(100);
    }
    drawCross(CROSS_CORNER_POINTS[i].x,CROSS_CORNER_POINTS[i].y,DISPLAY_BLACK);
    delay(1000);//Ensure that nobody is pressing the screen before we do the next point
  }

  //We can get nicer scaling if we allow more resolution on the divisor
  constexpr int32_t SCALE_SENSITIVITY_MULTIPLIER = 10;

  const int16_t diff_x_top = cal_points[1].x - cal_points[0].x;
  const int16_t diff_x_bottom = cal_points[3].x - cal_points[2].x;
  const int32_t diff_x_target = CROSS_CORNER_POINTS[1].x - CROSS_CORNER_POINTS[0].x;

  //Average the measured differences
  globalSettings.touchSlopeX = SCALE_SENSITIVITY_MULTIPLIER*(diff_x_top + diff_x_bottom) / (2*diff_x_target);

  const int16_t diff_y_left = cal_points[2].y - cal_points[0].y;
  const int16_t diff_y_right = cal_points[3].y - cal_points[1].y;
  const int32_t diff_y_target = CROSS_CORNER_POINTS[2].y - CROSS_CORNER_POINTS[0].y;

  //Average the measured differences
  globalSettings.touchSlopeY = SCALE_SENSITIVITY_MULTIPLIER*(diff_y_left + diff_y_right) / (2*diff_y_target);

  globalSettings.touchOffsetX = cal_points[0].x - ((CROSS_CORNER_OFFSET * globalSettings.touchSlopeX)/SCALE_SENSITIVITY_MULTIPLIER);
  globalSettings.touchOffsetY = cal_points[0].y - ((CROSS_CORNER_OFFSET * globalSettings.touchSlopeY)/SCALE_SENSITIVITY_MULTIPLIER);


/*
  for(uint8_t i = 0; i < sizeof(cal_points)/sizeof(cal_points[0]); ++i){
    Serial.print(cal_points[i].x);Serial.print(':');Serial.println(cal_points[i].y);
  }
  
  //for debugging
  Serial.print(globalSettings.touchSlopeX); Serial.print(' ');
  Serial.print(globalSettings.touchSlopeY); Serial.print(' ');
  Serial.print(globalSettings.touchOffsetX); Serial.print(' ');
  Serial.println(globalSettings.touchOffsetY); Serial.println(' ');
*/

  SaveSettingsToEeprom();
}
