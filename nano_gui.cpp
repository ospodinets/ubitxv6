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
static uint32_t displayGenerationCount = 0;

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

struct TextPlacement
{
  int cursorX;
  int cursorY;
};

static TextPlacement getTextPlacement(const char *text, int x, int y, int w, int h,
                                     TextJustification_e justification)
{
  int16_t boundsX;
  int16_t boundsY;
  uint16_t boundsWidth;
  uint16_t boundsHeight;
  tft.getTextBounds(text, x, y, &boundsX, &boundsY, &boundsWidth, &boundsHeight);

  if (TextJustification_e::Center == justification)
  {
    x += (w - ((int32_t)boundsWidth + (boundsX - x))) / 2;
  }
  else if (TextJustification_e::Right == justification)
  {
    x += w - ((int32_t)boundsWidth + (boundsX - x));
  }
  else
  {
    x += 2;
  }

  y += (h - static_cast<int32_t>(boundsHeight)) / 2 - (boundsY - y);
  return {x, y};
}

static void drawCustomGlyph(const GFXglyph *glyph, int x, int y, uint16_t color)
{
  const uint8_t glyphWidth = pgm_read_byte(&glyph->width);
  const uint8_t glyphHeight = pgm_read_byte(&glyph->height);
  const int8_t xOffset = static_cast<int8_t>(pgm_read_byte(&glyph->xOffset));
  const int8_t yOffset = static_cast<int8_t>(pgm_read_byte(&glyph->yOffset));
  uint16_t bitmapOffset = pgm_read_word(&glyph->bitmapOffset);
  const uint8_t *bitmap = static_cast<const uint8_t *>(pgm_read_ptr(&activeFont->bitmap));
  uint8_t bits = 0;
  uint8_t bit = 0;

  for (uint8_t row = 0; row < glyphHeight; ++row) {
    int8_t runStart = -1;
    for (uint8_t column = 0; column < glyphWidth; ++column) {
      if (!(bit++ & 7)) {
        bits = pgm_read_byte(&bitmap[bitmapOffset++]);
      }
      const bool pixelSet = (bits & 0x80) != 0;
      bits <<= 1;
      if (pixelSet) {
        if (runStart < 0) {
          runStart = column;
        }
      } else if (runStart >= 0) {
        const int runX = x + (xOffset + runStart) * activeTextSize;
        const int runY = y + (yOffset + row) * activeTextSize;
        const int runWidth = (column - runStart) * activeTextSize;
        if (activeTextSize == 1) {
          tft.writeFastHLine(runX, runY, runWidth, color);
        } else {
          tft.writeFillRect(runX, runY, runWidth, activeTextSize, color);
        }
        runStart = -1;
      }
    }
    if (runStart >= 0) {
      const int runX = x + (xOffset + runStart) * activeTextSize;
      const int runY = y + (yOffset + row) * activeTextSize;
      const int runWidth = (glyphWidth - runStart) * activeTextSize;
      if (activeTextSize == 1) {
        tft.writeFastHLine(runX, runY, runWidth, color);
      } else {
        tft.writeFillRect(runX, runY, runWidth, activeTextSize, color);
      }
    }
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

  tft.begin(40000000);
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
  ++displayGenerationCount;
  tft.fillRect(0,0,tft.width(),tft.height(),color);
}

uint32_t displayGeneration()
{
  return displayGenerationCount;
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
  const bool batchWrite = activeFont != nullptr;
  if (batchWrite) {
    tft.startWrite();
  }

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
        drawCustomGlyph(glyph, cursorX, cursorY, color);
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
  if (batchWrite) {
    tft.endWrite();
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

  const TextPlacement placement = getTextPlacement(text, x1, y1, w, h, justification);
  displayRawText(text, placement.cursorX, placement.cursorY, w, color, background);
}

bool displayTextUpdate(const char *previousText, const char *text, int x, int y,
                       int w, int h, const Style& style)
{
  setDisplayFont(getFont(style.fontFamily));
  setDisplayTextSize(style.fontSize);

  if (previousText == nullptr || text == nullptr) {
    return false;
  }
  if (strcmp(previousText, text) == 0) {
    return true;
  }
  if (activeFont == nullptr || strchr(previousText, '\n') != nullptr ||
      strchr(previousText, '\r') != nullptr || strchr(text, '\n') != nullptr ||
      strchr(text, '\r') != nullptr || activeTextSize != 1) {
    return false;
  }

  struct GlyphPlacement
  {
    unsigned char character;
    int16_t x;
    int16_t y;
    uint8_t width;
    uint8_t height;
  };
  constexpr size_t MAX_GLYPHS = 30;
  GlyphPlacement oldGlyphs[MAX_GLYPHS];
  GlyphPlacement newGlyphs[MAX_GLYPHS];
  size_t oldCount = 0;
  size_t newCount = 0;

  const uint16_t first = pgm_read_word(&activeFont->first);
  const uint16_t last = pgm_read_word(&activeFont->last);
  const GFXglyph *glyphs =
    static_cast<const GFXglyph *>(pgm_read_ptr(&activeFont->glyph));

  auto collectGlyphs = [&](const char *value, GlyphPlacement *placements,
                           size_t& count) -> bool {
    if (strlen(value) >= MAX_GLYPHS) {
      return false;
    }

    const TextPlacement placement =
      getTextPlacement(value, x, y, w, h, style.justification);
    int cursorX = placement.cursorX;
    for (size_t i = 0; value[i] != '\0'; ++i) {
      const unsigned char character = static_cast<unsigned char>(value[i]);
      if (character < first || character > last) {
        continue;
      }

      const GFXglyph *glyph = &glyphs[character - first];
      const uint8_t glyphWidth = pgm_read_byte(&glyph->width);
      const uint8_t glyphHeight = pgm_read_byte(&glyph->height);
      const int8_t xOffset = static_cast<int8_t>(pgm_read_byte(&glyph->xOffset));
      const int8_t yOffset = static_cast<int8_t>(pgm_read_byte(&glyph->yOffset));
      const int glyphX = cursorX + xOffset;
      const int glyphY = placement.cursorY + yOffset;
      const int minX = style.borderColor == style.backgroundColor ? x : x + 1;
      const int minY = style.borderColor == style.backgroundColor ? y : y + 1;
      const int maxX = style.borderColor == style.backgroundColor ? x + w : x + w - 1;
      const int maxY = style.borderColor == style.backgroundColor ? y + h : y + h - 1;
      if (glyphWidth == 0 || glyphHeight == 0 ||
          glyphX < minX || glyphX + glyphWidth > maxX ||
          glyphY < minY || glyphY + glyphHeight > maxY ||
          glyphX < 0 || glyphY < 0 ||
          glyphX + glyphWidth > tft.width() ||
          glyphY + glyphHeight > tft.height()) {
        return false;
      }

      placements[count++] = {
        character,
        static_cast<int16_t>(glyphX),
        static_cast<int16_t>(glyphY),
        glyphWidth,
        glyphHeight
      };
      cursorX += pgm_read_byte(&glyph->xAdvance);
    }
    return true;
  };

  if (!collectGlyphs(previousText, oldGlyphs, oldCount) ||
      !collectGlyphs(text, newGlyphs, newCount)) {
    return false;
  }

  auto sameGlyphAtPosition = [](const GlyphPlacement& glyph,
                                const GlyphPlacement *others,
                                size_t otherCount) {
    for (size_t i = 0; i < otherCount; ++i) {
      if (glyph.character == others[i].character &&
          glyph.x == others[i].x && glyph.y == others[i].y) {
        return true;
      }
    }
    return false;
  };
  auto intersects = [](const GlyphPlacement& a, const GlyphPlacement& b) {
    return a.x < b.x + b.width && b.x < a.x + a.width &&
           a.y < b.y + b.height && b.y < a.y + a.height;
  };

  bool hasChanges = false;
  for (size_t i = 0; i < oldCount; ++i) {
    hasChanges |= !sameGlyphAtPosition(oldGlyphs[i], newGlyphs, newCount);
  }
  for (size_t i = 0; i < newCount; ++i) {
    hasChanges |= !sameGlyphAtPosition(newGlyphs[i], oldGlyphs, oldCount);
  }
  if (!hasChanges) {
    return true;
  }

  tft.startWrite();
  for (size_t i = 0; i < oldCount; ++i) {
    if (!sameGlyphAtPosition(oldGlyphs[i], newGlyphs, newCount)) {
      tft.writeFillRect(oldGlyphs[i].x, oldGlyphs[i].y,
                        oldGlyphs[i].width, oldGlyphs[i].height,
                        style.backgroundColor);
    }
  }
  for (size_t i = 0; i < newCount; ++i) {
    if (!sameGlyphAtPosition(newGlyphs[i], oldGlyphs, oldCount)) {
      tft.writeFillRect(newGlyphs[i].x, newGlyphs[i].y,
                        newGlyphs[i].width, newGlyphs[i].height,
                        style.backgroundColor);
    }
  }

  for (size_t i = 0; i < newCount; ++i) {
    bool redraw = !sameGlyphAtPosition(newGlyphs[i], oldGlyphs, oldCount);
    if (!redraw) {
      for (size_t j = 0; j < oldCount && !redraw; ++j) {
        if (!sameGlyphAtPosition(oldGlyphs[j], newGlyphs, newCount) &&
            intersects(newGlyphs[i], oldGlyphs[j])) {
          redraw = true;
        }
      }
      for (size_t j = 0; j < newCount && !redraw; ++j) {
        if (!sameGlyphAtPosition(newGlyphs[j], oldGlyphs, oldCount) &&
            intersects(newGlyphs[i], newGlyphs[j])) {
          redraw = true;
        }
      }
    }
    if (redraw) {
      const GFXglyph *glyph = &glyphs[newGlyphs[i].character - first];
      drawCustomGlyph(glyph, newGlyphs[i].x -
                      static_cast<int8_t>(pgm_read_byte(&glyph->xOffset)),
                      newGlyphs[i].y -
                      static_cast<int8_t>(pgm_read_byte(&glyph->yOffset)),
                      style.textColor);
    }
  }
  tft.endWrite();
  return true;
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
    Serial.print(F("TOUCH: calibration point "));
    Serial.println(i + 1);
    drawCross(CROSS_CORNER_POINTS[i].x,CROSS_CORNER_POINTS[i].y,DISPLAY_WHITE);
    while(!readTouch(&cal_points[i])){
      if(IsButtonPressed()){
        WaitForTunerButtonRelease();//Consume the press so that it isn't seen by the next screen
        Serial.println(F("TOUCH: calibration cancelled"));
        return;
      }
      delay(100);
    }
    Serial.print(F("TOUCH: calibration raw x="));
    Serial.print(cal_points[i].x);
    Serial.print(F(" y="));
    Serial.println(cal_points[i].y);
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

  Serial.print(F("TOUCH: calibration slope x="));
  Serial.print(globalSettings.touchSlopeX);
  Serial.print(F(" y="));
  Serial.print(globalSettings.touchSlopeY);
  Serial.print(F(" offset x="));
  Serial.print(globalSettings.touchOffsetX);
  Serial.print(F(" y="));
  Serial.println(globalSettings.touchOffsetY);

  SaveSettingsToEeprom();
}
