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
    m_style(style), m_dirty(true)
{
}

void Label::setText(const char *text)
{
    if (m_text_override == nullptr &&
        (m_text == text || (m_text != nullptr && text != nullptr && strcmp(m_text, text) == 0)))
    {
        return;
    }

    m_text = text;
    m_text_override = nullptr;
    m_dirty = true;
}

void Label::setTextOverride(LabelTextOverride_t text_override)
{
    if (m_text_override == text_override && m_text == nullptr)
    {
        return;
    }

    m_text = nullptr;
    m_text_override = text_override;
    m_dirty = true;
}

void Label::setStyle(Style *style)
{
    m_style = style;
    m_dirty = true;
}

void Label::draw()
{
    Style *style = m_style ? m_style : &defaultLabelStyle;

    if (nullptr != m_text)
    {
        if (!m_dirty)
        {
            return;
        }

        displayText(m_text, m_x, m_y, m_w, m_h, *style);
    }
    else if (nullptr != m_text_override)
    {
        char text[30] = {0};
        m_text_override(text, sizeof(text));
        text[sizeof(text) - 1] = '\0';

        if (!m_dirty && strcmp(m_last_drawn_text, text) == 0)
        {
            return;
        }

        displayText(text, m_x, m_y, m_w, m_h, *style);
        memcpy(m_last_drawn_text, text, sizeof(m_last_drawn_text));
        m_last_drawn_text[sizeof(m_last_drawn_text) - 1] = '\0';
    }

    m_dirty = false;
}
