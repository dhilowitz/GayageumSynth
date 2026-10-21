#include "Theme.h"

namespace
{
    juce::Typeface::Ptr load (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    struct Typefaces
    {
        juce::Typeface::Ptr uiMedium   = load (BinaryData::BarlowCondensedMedium_ttf, BinaryData::BarlowCondensedMedium_ttfSize);
        juce::Typeface::Ptr uiSemibold = load (BinaryData::BarlowCondensedSemiBold_ttf, BinaryData::BarlowCondensedSemiBold_ttfSize);
        juce::Typeface::Ptr uiBold     = load (BinaryData::BarlowCondensedBold_ttf, BinaryData::BarlowCondensedBold_ttfSize);
        juce::Typeface::Ptr monoRegular  = load (BinaryData::JetBrainsMonoRegular_ttf, BinaryData::JetBrainsMonoRegular_ttfSize);
        juce::Typeface::Ptr monoSemibold = load (BinaryData::JetBrainsMonoSemiBold_ttf, BinaryData::JetBrainsMonoSemiBold_ttfSize);
    };

    Typefaces& typefaces()
    {
        static Typefaces t;
        return t;
    }
}

//==============================================================================
juce::Font Theme::ui (float height, Weight weight)
{
    auto& t = typefaces();
    auto typeface = weight == Weight::bold ? t.uiBold : weight == Weight::medium ? t.uiMedium : t.uiSemibold;
    return juce::Font (juce::FontOptions (typeface).withHeight (height));
}

juce::Font Theme::mono (float height, bool semibold)
{
    auto& t = typefaces();
    return juce::Font (juce::FontOptions (semibold ? t.monoSemibold : t.monoRegular).withHeight (height));
}

void Theme::drawLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                       juce::Justification justification, float height)
{
    g.setColour (Colours::label);
    g.setFont (ui (height, Weight::bold).withExtraKerningFactor (0.14f));
    g.drawText (text.toUpperCase(), area, justification, false);
}

//==============================================================================
Theme::LookAndFeel::LookAndFeel()
{
    using namespace Colours;

    setColourScheme ({ ground, ground, panel, border, text, accent, text, button, text });

    setColour (juce::ResizableWindow::backgroundColourId, ground);

    setColour (juce::TextButton::buttonColourId, button);
    setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.35f));
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, text);
    setColour (juce::ComboBox::outlineColourId, border);
    setColour (juce::ComboBox::backgroundColourId, inset);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::arrowColourId, textSecondary);

    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::headerTextColourId, label);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff2a3142));
    setColour (juce::PopupMenu::highlightedTextColourId, text);

    setColour (juce::Label::textColourId, text);
    setColour (juce::TextEditor::backgroundColourId, inset);
    setColour (juce::TextEditor::outlineColourId, insetBorder);
    setColour (juce::TextEditor::focusedOutlineColourId, accent);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, accent);

    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, inset);
    setColour (juce::Slider::textBoxOutlineColourId, insetBorder);
    setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.35f));
    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Slider::rotarySliderOutlineColourId, knobTrack);
    setColour (juce::Slider::thumbColourId, accent);
    setColour (juce::Slider::trackColourId, accent.withAlpha (0.6f));
    setColour (juce::Slider::backgroundColourId, well);

    setColour (juce::TooltipWindow::backgroundColourId, panel);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::TooltipWindow::outlineColourId, border);
}

juce::Typeface::Ptr Theme::LookAndFeel::getTypefaceForFont (const juce::Font& font)
{
    // Fonts asked for by generic name (JUCE's own widgets) get the theme's typefaces
    auto& t = typefaces();
    auto name = font.getTypefaceName();

    if (name == juce::Font::getDefaultMonospacedFontName())
        return font.isBold() ? t.monoSemibold : t.monoRegular;

    if (name == juce::Font::getDefaultSansSerifFontName() || name == juce::Font::getDefaultSerifFontName())
        return font.isBold() ? t.uiBold : t.uiMedium;

    return LookAndFeel_V4::getTypefaceForFont (font);
}

void Theme::LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                                               bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto colour = backgroundColour.withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f);

    if (isButtonDown)
        colour = colour.brighter (0.15f);
    else if (isMouseOverButton)
        colour = colour.brighter (0.07f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (Colours::border.withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.5f));
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
}

juce::Font Theme::LookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return ui (juce::jlimit (13.0f, 17.0f, (float) buttonHeight * 0.55f));
}

void Theme::LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           float startAngle, float endAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
    auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto centre = bounds.getCentre();
    auto stroke = juce::jmax (2.5f, size * 0.09f);
    auto radius = size * 0.5f - stroke * 0.5f - 1.0f;   // the arc's whole stroke inside the knob's bounds
    auto angle = startAngle + sliderPos * (endAngle - startAngle);
    auto origin = (float) slider.getProperties().getWithDefault ("origin", 0.0);
    auto fromAngle = startAngle + origin * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::knobTrack);
    g.strokePath (track, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    if (std::abs (angle - fromAngle) > 0.01f && slider.isEnabled())
    {
        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (value, juce::PathStrokeType (stroke, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    auto faceRadius = radius * 0.62f;
    g.setColour (Colours::knobFace);
    g.fillEllipse (centre.x - faceRadius, centre.y - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f);
    g.setColour (Colours::knobEdge);
    g.drawEllipse (centre.x - faceRadius, centre.y - faceRadius, faceRadius * 2.0f, faceRadius * 2.0f, 1.0f);

    auto tip = centre.getPointOnCircumference (faceRadius * 0.85f, angle);
    g.setColour (Colours::text.withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.4f));
    g.drawLine ({ centre, tip }, 2.0f);
}

void Theme::LookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                           float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style == juce::Slider::LinearBarVertical)
    {
        auto box = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (0.5f);
        g.setColour (Colours::inset);
        g.fillRoundedRectangle (box, 3.0f);
        g.setColour (Colours::insetBorder);
        g.drawRoundedRectangle (box, 3.0f, 1.0f);
        return;
    }

    if (style == juce::Slider::LinearVertical)
    {
        auto centreX = (float) x + (float) width * 0.5f;
        auto track = juce::Rectangle<float> (centreX - 2.5f, (float) y, 5.0f, (float) height);
        g.setColour (Colours::well);
        g.fillRoundedRectangle (track, 2.5f);
        g.setColour (Colours::divider);
        g.drawRoundedRectangle (track, 2.5f, 1.0f);
        if (slider.isEnabled())
        {
            g.setColour (slider.findColour (juce::Slider::trackColourId));
            g.fillRoundedRectangle (track.withTop (sliderPos).reduced (1.0f), 2.0f);
        }

        auto capWidth = juce::jmin (36.0f, (float) width - 2.0f);
        auto cap = juce::Rectangle<float> (capWidth, 16.0f).withCentre ({ centreX, sliderPos });
        g.setColour (Colours::faderCap);
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (Colours::faderCapEdge);
        g.drawRoundedRectangle (cap, 3.0f, 1.0f);
        g.setColour (slider.findColour (juce::Slider::thumbColourId).withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.4f));
        g.fillRect (cap.withSizeKeepingCentre (cap.getWidth() - 6.0f, 2.0f));
        return;
    }

    if (style != juce::Slider::LinearHorizontal)
    {
        LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        return;
    }

    auto centreY = (float) y + (float) height * 0.5f;
    auto track = juce::Rectangle<float> ((float) x, centreY - 2.5f, (float) width, 5.0f);
    g.setColour (Colours::well);
    g.fillRoundedRectangle (track, 2.5f);
    g.setColour (Colours::divider);
    g.drawRoundedRectangle (track, 2.5f, 1.0f);
    g.setColour (slider.findColour (juce::Slider::trackColourId));
    g.fillRoundedRectangle (track.withRight (sliderPos).reduced (1.0f), 2.0f);

    auto capHeight = juce::jmin (24.0f, (float) height - 2.0f);
    auto cap = juce::Rectangle<float> (14.0f, capHeight).withCentre ({ sliderPos, centreY });
    g.setColour (Colours::faderCap);
    g.fillRoundedRectangle (cap, 3.0f);
    g.setColour (Colours::faderCapEdge);
    g.drawRoundedRectangle (cap, 3.0f, 1.0f);
    g.setColour (slider.findColour (juce::Slider::thumbColourId));
    g.fillRect (cap.withSizeKeepingCentre (2.0f, cap.getHeight() - 6.0f));
}

juce::Label* Theme::LookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    // Values in JetBrains Mono, in an inset readout (LinearBarVertical draws its own box)
    auto* l = LookAndFeel_V4::createSliderTextBox (slider);
    l->setFont (mono (12.0f));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, Colours::text);

    if (slider.getSliderStyle() == juce::Slider::LinearBarVertical)
    {
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    }
    else
    {
        l->setColour (juce::Label::backgroundColourId, Colours::inset);
        l->setColour (juce::Label::outlineColourId, Colours::insetBorder);
    }
    return l;
}

juce::Font Theme::LookAndFeel::getPopupMenuFont()                { return ui (16.0f, Weight::medium); }
juce::Font Theme::LookAndFeel::getComboBoxFont (juce::ComboBox&) { return ui (16.0f, Weight::medium); }
juce::Font Theme::LookAndFeel::getLabelFont (juce::Label& label)
{
    // Labels that set their own font keep it; the default (15 px sans) becomes the theme's
    auto f = label.getFont();
    return f.getTypefaceName() == juce::Font::getDefaultSansSerifFontName() ? ui (juce::jmax (15.0f, f.getHeight()), Weight::medium) : f;
}
