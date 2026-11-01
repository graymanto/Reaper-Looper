#include "ClipFactory.h"
#include "MidiClip.h"

namespace reaperlooper::ClipFactory
{

std::unique_ptr<Clip> createDefault (const juce::String& typeId)
{
    if (typeId == "Midi")
        return std::make_unique<MidiClip>();

    // Audio clips (Phase 6) will be added here.
    return nullptr;
}

std::unique_ptr<Clip> fromValueTree (const juce::ValueTree& tree)
{
    if (! tree.hasType ("Clip"))
        return nullptr;

    const auto typeId = tree.getProperty ("type", juce::String()).toString();
    auto clip = createDefault (typeId);
    if (clip != nullptr)
        clip->fromValueTree (tree);

    return clip;
}

} // namespace reaperlooper::ClipFactory
