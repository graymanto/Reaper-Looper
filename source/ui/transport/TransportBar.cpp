#include "TransportBar.h"
#include <array>

namespace reaperlooper
{

namespace
{
    constexpr std::array<std::pair<const char*, LaunchQuantize>, 7> kQuantizeChoices {{
        { "None", LaunchQuantize::None },
        { "1/4 note", LaunchQuantize::Quarter },
        { "1/2 note", LaunchQuantize::Half },
        { "1 bar", LaunchQuantize::OneBar },
        { "2 bars", LaunchQuantize::TwoBars },
        { "4 bars", LaunchQuantize::FourBars },
        { "8 bars", LaunchQuantize::EightBars },
    }};
}

TransportBar::TransportBar (Callbacks cbs) : cbs_ (std::move (cbs))
{
    positionLabel_.setFont (Theme::monoFont (15.0f, "Medium"));
    positionLabel_.setColour (juce::Label::textColourId, Theme::textPrimary);
    positionLabel_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (positionLabel_);

    bpmLabel_.setFont (Theme::monoFont (13.0f));
    bpmLabel_.setColour (juce::Label::textColourId, Theme::textSecondary);
    bpmLabel_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (bpmLabel_);

    recordButton_.setColour (juce::TextButton::buttonOnColourId, Theme::recordRed);
    recordButton_.onClick = [this] { if (cbs_.onToggleRecord) cbs_.onToggleRecord(); };
    addAndMakeVisible (recordButton_);

    for (const auto& [name, q] : kQuantizeChoices)
        quantizeCombo_.addItem (name, static_cast<int> (q) + 1); // +1: JUCE item ids must be > 0, None == 0
    quantizeCombo_.setSelectedId (static_cast<int> (LaunchQuantize::OneBar) + 1, juce::dontSendNotification);
    quantizeCombo_.onChange = [this]
    {
        if (cbs_.onGlobalQuantizeChanged)
            cbs_.onGlobalQuantizeChanged (static_cast<LaunchQuantize> (quantizeCombo_.getSelectedId() - 1));
    };
    addAndMakeVisible (quantizeCombo_);

    statusLabel_.setFont (Theme::monoFont (11.0f));
    statusLabel_.setColour (juce::Label::textColourId, Theme::textDim);
    statusLabel_.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (statusLabel_);
}

void TransportBar::refresh (const HostTransportState& transport, const juce::String& reaperStatus,
                             const juce::String& hardwareStatus)
{
    const int bar = transport.barIndex + 1;
    const double beatInBar = transport.positionQuarterNotes - transport.barStartQuarterNotes + 1.0;

    juce::String posText;
    posText << (transport.isRecording ? "REC  " : (transport.isPlaying ? "> " : "|| "))
            << "Bar " << bar << "  Beat " << juce::String (beatInBar, 2);
    positionLabel_.setText (posText, juce::dontSendNotification);

    bpmLabel_.setText (juce::String (transport.bpm, 1) + " bpm", juce::dontSendNotification);

    recordButton_.setToggleState (transport.isRecording, juce::dontSendNotification);

    statusLabel_.setText (reaperStatus + "   " + hardwareStatus, juce::dontSendNotification);
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds().reduced (6);
    positionLabel_.setBounds (bounds.removeFromLeft (220));
    bpmLabel_.setBounds (bounds.removeFromLeft (100));
    bounds.removeFromLeft (8);
    recordButton_.setBounds (bounds.removeFromLeft (56));
    bounds.removeFromLeft (8);
    quantizeCombo_.setBounds (bounds.removeFromLeft (140));
    statusLabel_.setBounds (bounds);
}

void TransportBar::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (Theme::backgroundDeep, 0.0f, 0.0f,
                                              Theme::surface.withAlpha (0.96f), 0.0f, static_cast<float> (getHeight()), false));
    g.fillAll();
    g.setColour (Theme::borderHi.withAlpha (0.70f));
    g.drawLine (0.0f, static_cast<float> (getHeight()) - 0.5f, static_cast<float> (getWidth()), static_cast<float> (getHeight()) - 0.5f);
}

} // namespace reaperlooper
