#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;

    // Layout (px)
    constexpr int kPad = 12, kGap = 10;
    constexpr int kHeaderHeight = 48;
    constexpr int kCellWidth = 76, kCellHeight = 98;
    constexpr int kStringWidth = 52;
    constexpr int kSectionTitle = 26, kSectionBottom = 10;
    constexpr int kSectionHeight = kSectionTitle + kCellHeight + kSectionBottom;
    constexpr int kStringsWidth = GayageumSynthAudioProcessor::numStrings * kStringWidth + 2 * kGap;
    constexpr int kRightWidth = 3 * kCellWidth + 2 * kGap;          // Tone has three controls
    constexpr int kBodyHeight = 2 * kSectionHeight + kGap;          // Tone and Doubling, beside Strings
    constexpr int kWidth = kPad + kStringsWidth + kGap + kRightWidth + kPad;
    constexpr int kHeight = kHeaderHeight + kPad + kBodyHeight + kPad;

    // Readouts with their units; typed values are read back in the same units
    void showPercent (juce::Slider& slider)
    {
        slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
        slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue() / 100.0; };
        slider.updateText();
    }

    void showSemitones (juce::Slider& slider)
    {
        slider.textFromValueFunction = [] (double v) { return (v > 0.0 ? "+" : "") + juce::String (v, 2) + "st"; };
        slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
        slider.updateText();
    }

    void showMetres (juce::Slider& slider)
    {
        slider.textFromValueFunction = [] (double v) { return juce::String (v, 3) + "m"; };
        slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
        slider.updateText();
    }

    // "E3", or "E3+12" when it's 12 cents sharp
    juce::String noteName (float frequency)
    {
        const double note = 69.0 + 12.0 * std::log2 (frequency / 440.0);
        const int nearest = juce::roundToInt (note);
        const int cents = juce::roundToInt ((note - nearest) * 100.0);
        auto name = juce::MidiMessage::getMidiNoteName (nearest, true, true, 4);
        return cents == 0 ? name : name + (cents > 0 ? "+" : "") + juce::String (cents);
    }
}

//==============================================================================
GayageumSynthAudioProcessorEditor::Cell::Cell (const juce::String& n) : name (n) {}

void GayageumSynthAudioProcessorEditor::Cell::paint (juce::Graphics& g)
{
    Theme::drawLabel (g, name, getLocalBounds().removeFromTop (nameHeight), juce::Justification::centred, 11.5f);
}

juce::Rectangle<int> GayageumSynthAudioProcessorEditor::Cell::controlArea() const
{
    return getLocalBounds().withTrimmedTop (nameHeight).withTrimmedBottom (readoutHeight + gap);
}

juce::Rectangle<int> GayageumSynthAudioProcessorEditor::Cell::readoutArea() const
{
    return getLocalBounds().removeFromBottom (readoutHeight).reduced (3, 0);
}

//==============================================================================
GayageumSynthAudioProcessorEditor::Knob::Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                                               const juce::String& name, const juce::String& tooltip, float origin)
    : Cell (name)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, kCellWidth - 6, readoutHeight);
    slider.getProperties().set ("origin", origin);
    slider.setTooltip (tooltip);
    attachment = std::make_unique<SA> (apvts, paramID, slider);
    slider.setDoubleClickReturnValue (true, apvts.getParameterRange (paramID).convertFrom0to1 (apvts.getParameter (paramID)->getDefaultValue()));
    addAndMakeVisible (slider);
}

void GayageumSynthAudioProcessorEditor::Knob::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (nameHeight));
}

//==============================================================================
GayageumSynthAudioProcessorEditor::StringCell::StringCell (juce::AudioProcessorValueTreeState& apvts, int index)
    : Cell (juce::String (index + 1)), stringIndex (index)
{
    const auto paramID = "anjok" + juce::String (index + 1);

    slider.setSliderStyle (juce::Slider::LinearVertical);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, kStringWidth - 6, readoutHeight);
    slider.setTooltip ("String " + juce::String (index + 1) + " Anjok: move the bridge to retune the string");
    attachment = std::make_unique<SA> (apvts, paramID, slider);
    showMetres (slider);
    slider.setDoubleClickReturnValue (true, apvts.getParameterRange (paramID).convertFrom0to1 (apvts.getParameter (paramID)->getDefaultValue()));
    slider.onValueChange = [this] { repaint (0, nameHeight, getWidth(), pitchHeight); };
    addAndMakeVisible (slider);
}

void GayageumSynthAudioProcessorEditor::StringCell::resized()
{
    slider.setBounds (getLocalBounds().withTrimmedTop (nameHeight + pitchHeight));
}

void GayageumSynthAudioProcessorEditor::StringCell::paint (juce::Graphics& g)
{
    Cell::paint (g);

    // The note the string plays, from where its Anjok is
    g.setColour (Theme::Colours::textSecondary);
    g.setFont (Theme::mono (11.0f));
    g.drawText (noteName (GayageumSynthAudioProcessor::stringFrequency (stringIndex, (float) slider.getValue())),
                getLocalBounds().withTrimmedTop (nameHeight).withHeight (pitchHeight), juce::Justification::centred, false);
}

//==============================================================================
void GayageumSynthAudioProcessorEditor::RoundButton::paintButton (juce::Graphics& g, bool isMouseOver, bool isButtonDown)
{
    auto bounds = getLocalBounds().toFloat();
    auto diameter = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.66f;
    auto circle = juce::Rectangle<float> (diameter, diameter).withCentre (bounds.getCentre());
    auto on = getToggleState();

    auto face = on ? Theme::Colours::accent.withAlpha (0.85f) : Theme::Colours::knobFace;
    if (isButtonDown)      face = face.brighter (0.2f);
    else if (isMouseOver)  face = face.brighter (0.08f);

    g.setColour (Theme::Colours::knobTrack);
    g.drawEllipse (circle.expanded (3.0f), 2.5f);
    g.setColour (face);
    g.fillEllipse (circle);
    g.setColour (on ? Theme::Colours::accent.brighter (0.3f) : Theme::Colours::knobEdge);
    g.drawEllipse (circle, 1.0f);
}

//==============================================================================
GayageumSynthAudioProcessorEditor::ToggleCell::ToggleCell (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID,
                                                           const juce::String& name, const juce::String& tooltip)
    : Cell (name)
{
    button.setClickingTogglesState (true);
    button.setTooltip (tooltip);
    button.onStateChange = [this] { repaint(); };
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, paramID, button);
    addAndMakeVisible (button);
}

void GayageumSynthAudioProcessorEditor::ToggleCell::resized()
{
    button.setBounds (controlArea());
}

void GayageumSynthAudioProcessorEditor::ToggleCell::paint (juce::Graphics& g)
{
    Cell::paint (g);

    auto box = readoutArea().toFloat().reduced (0.5f);
    g.setColour (Theme::Colours::inset);
    g.fillRect (box);
    g.setColour (Theme::Colours::insetBorder);
    g.drawRect (box, 1.0f);
    g.setColour (Theme::Colours::text);
    g.setFont (Theme::mono (12.0f));
    g.drawText (button.getToggleState() ? "on" : "off", readoutArea(), juce::Justification::centred, false);
}

//==============================================================================
GayageumSynthAudioProcessorEditor::GayageumSynthAudioProcessorEditor (GayageumSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      excitation    (p.apvts, "excitationBlend", "Excitation",
                     "Excitation: from a smooth, rounded pluck (0%) to a bright, noisy one (100%)"),
      body          (p.apvts, "bodyResonance",   "Body",
                     "Body Resonance: how much the instrument's wooden body colours the sound"),
      damping       (p.apvts, "damping",         "Damping",
                     "Damping: how long the strings ring, from short (0%) to long (100%)"),
      doubleStrings (p.apvts, "doubleStrings",   "Double",
                     "Double Strings: play a second, detuned string with every note"),
      detune        (p.apvts, "doubleDetune",    "Detune",
                     "Double Detune: the second string's tuning, in semitones", 0.5f)
{
    // Play mode: two buttons on one parameter
    for (auto* button : { &traditionalButton, &freePlayButton })
    {
        button->setRadioGroupId (1);
        addAndMakeVisible (button);
    }
    traditionalButton.setTooltip ("Traditional: notes C4 to B4 play the twelve strings, tuned by their Anjoks");
    freePlayButton.setTooltip ("Free Play: every note plays at its own pitch, up to 16 at once");

    playModeAttachment = std::make_unique<juce::ParameterAttachment> (*p.apvts.getParameter ("playMode"),
                                                                      [this] (float value) { showMode (value > 0.5f); });
    traditionalButton.onClick = [this] { playModeAttachment->setValueAsCompleteGesture (0.0f); };
    freePlayButton.onClick    = [this] { playModeAttachment->setValueAsCompleteGesture (1.0f); };

    for (int i = 0; i < GayageumSynthAudioProcessor::numStrings; ++i)
    {
        strings[(size_t) i] = std::make_unique<StringCell> (p.apvts, i);
        addAndMakeVisible (*strings[(size_t) i]);
    }

    for (auto* c : std::initializer_list<juce::Component*> { &excitation, &body, &damping, &doubleStrings, &detune })
        addAndMakeVisible (c);

    // After the attachments, which set the parameters' own text (without units)
    showPercent (excitation.slider);
    showPercent (body.slider);
    showPercent (damping.slider);
    showSemitones (detune.slider);

    playModeAttachment->sendInitialUpdate();

    // Last, so it reaches every component added above
    setLookAndFeel (&lookAndFeel);

    setSize (kWidth, kHeight);
}

GayageumSynthAudioProcessorEditor::~GayageumSynthAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void GayageumSynthAudioProcessorEditor::showMode (bool freePlay)
{
    freePlayMode = freePlay;
    traditionalButton.setToggleState (! freePlay, juce::dontSendNotification);
    freePlayButton.setToggleState (freePlay, juce::dontSendNotification);

    // The Anjoks only tune the strings in Traditional mode, and doubling only happens in Free Play
    auto setActive = [] (juce::Component& c, bool active)
    {
        c.setEnabled (active);
        c.setAlpha (active ? 1.0f : 0.4f);
    };

    for (auto& s : strings)
        setActive (*s, ! freePlay);
    setActive (doubleStrings, freePlay);
    setActive (detune, freePlay);

    repaint();
}

//==============================================================================
void GayageumSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::Colours::ground);

    // Title bar
    auto header = getLocalBounds().removeFromTop (kHeaderHeight);
    g.setColour (Theme::Colours::bar);
    g.fillRect (header);
    g.setColour (Theme::Colours::divider);
    g.drawHorizontalLine (kHeaderHeight - 1, 0.0f, (float) getWidth());

    auto title = header.reduced (kPad, 0);
    g.setFont (Theme::ui (26.0f, Theme::Weight::bold).withExtraKerningFactor (0.06f));
    g.setColour (Theme::Colours::text);
    g.drawText ("GAYAGEUM", title, juce::Justification::centredLeft, false);
    const int nameWidth = juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), "GAYAGEUM");
    g.setColour (Theme::Colours::accent);
    g.drawText ("SYNTH", title.withTrimmedLeft (nameWidth + 1), juce::Justification::centredLeft, false);

    Theme::drawLabel (g, "Mode", traditionalButton.getBounds().withX (traditionalButton.getX() - 48).withWidth (40),
                      juce::Justification::centredRight, 11.5f);

    for (auto& section : sections)
    {
        auto area = section.area.toFloat();
        g.setColour (Theme::Colours::panel);
        g.fillRoundedRectangle (area, 6.0f);
        g.setColour (Theme::Colours::border);
        g.drawRoundedRectangle (area.reduced (0.5f), 6.0f, 1.0f);

        auto titleArea = section.area.withHeight (kSectionTitle).reduced (kGap, 0);
        Theme::drawLabel (g, section.title, titleArea);

        // What the section does in the current mode, at the right of its title
        juce::String note;
        if (section.title == "Strings")
            note = freePlayMode ? "Not used in Free Play" : "C4 to B4 play strings 1 to 12";
        else if (section.title == "Doubling" && ! freePlayMode)
            note = "Free Play only";

        g.setColour (Theme::Colours::dim);
        g.setFont (Theme::ui (14.0f, Theme::Weight::medium));
        g.drawText (note, titleArea, juce::Justification::centredRight, false);
    }
}

void GayageumSynthAudioProcessorEditor::resized()
{
    // Title bar: the play mode at the right
    auto header = getLocalBounds().removeFromTop (kHeaderHeight).reduced (kPad, 10);
    freePlayButton.setBounds (header.removeFromRight (96));
    header.removeFromRight (4);
    traditionalButton.setBounds (header.removeFromRight (96));

    sections.clear();
    auto content = getLocalBounds().withTrimmedTop (kHeaderHeight).reduced (kPad);
    auto left = content.removeFromLeft (kStringsWidth);
    content.removeFromLeft (kGap);
    auto right = content;

    auto addSection = [this] (juce::Rectangle<int>& column, const juce::String& title, int height)
    {
        auto area = column.removeFromTop (height);
        column.removeFromTop (kGap);
        sections.push_back ({ title, area });
        return area.withTrimmedTop (kSectionTitle).withTrimmedBottom (kSectionBottom).reduced (kGap, 0);
    };

    auto placeCells = [] (juce::Rectangle<int> row, std::initializer_list<juce::Component*> cells)
    {
        for (auto* c : cells)
            c->setBounds (row.removeFromLeft (kCellWidth));
    };

    // Left: the strings, lowest first
    auto stringsRow = addSection (left, "Strings", kBodyHeight);
    for (auto& s : strings)
        s->setBounds (stringsRow.removeFromLeft (kStringWidth));

    // Right: Tone, Doubling
    placeCells (addSection (right, "Tone", kSectionHeight),     { &excitation, &body, &damping });
    placeCells (addSection (right, "Doubling", kSectionHeight), { &doubleStrings, &detune });
}
