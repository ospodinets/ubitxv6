#include "label.h"

#include "color_theme.h"
#include "nano_gui.h"

#include <string.h>

Style defaultLabelStyle = {
    COLOR_INACTIVE_TEXT,
    COLOR_INACTIVE_BACKGROUND,
    COLOR_INACTIVE_BORDER,
    TextJustification_e::Center,
    FontFamily_e::Regular,
    1};

Label::Label(int16_t x, int16_t y, int16_t w, int16_t h, const char *text,
             LabelTextOverride_t text_override, Style *style)
    : m_x(x), m_y(y), m_w(w), m_h(h), m_text(text), m_text_override(text_override), 
      m_style(style), m_dirty(true), m_has_drawn_text(false)
{
}

void Label::setText(const char *text)
{
      m_text = text;
      m_text_override = nullptr;
}

void Label::setTextOverride(LabelTextOverride_t text_override)
{
      m_text = nullptr;
      m_text_override = text_override;
}

void Label::setStyle(Style *style)
{
      m_style = style;
}

void Label::invalidate()
{
      m_dirty = true;
}

void Label::draw()
{
    Style *style = m_style ? m_style : &defaultLabelStyle;
    char textBuffer[sizeof(m_last_drawn_text)] = {};
    const char *text = m_text;

    if (m_text_override != nullptr) {
      m_text_override(textBuffer, sizeof(textBuffer));
      textBuffer[sizeof(textBuffer) - 1] = '\0';
      text = textBuffer;
    }
    if (text == nullptr) {
      return;
    }

    const bool cacheable = strlen(text) < sizeof(m_last_drawn_text);
    const bool styleChanged =
      m_last_style.textColor != style->textColor ||
      m_last_style.backgroundColor != style->backgroundColor ||
      m_last_style.borderColor != style->borderColor ||
      m_last_style.justification != style->justification ||
      m_last_style.fontFamily != style->fontFamily ||
      m_last_style.fontSize != style->fontSize;
    const bool forceRedraw = m_dirty || !m_has_drawn_text || styleChanged || !cacheable;

    if (!forceRedraw && strcmp(m_last_drawn_text, text) == 0) {
      return;
    }

    if (forceRedraw ||
        !displayTextUpdate(m_last_drawn_text, text, m_x, m_y, m_w, m_h, *style)) {
      displayText(text, m_x, m_y, m_w, m_h, *style);
    }

    if (cacheable) {
      memcpy(m_last_drawn_text, text, strlen(text) + 1);
      m_has_drawn_text = true;
    } else {
      m_has_drawn_text = false;
    }
    m_last_style = *style;
    m_dirty = false;
}
