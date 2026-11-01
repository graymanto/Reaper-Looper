#pragma once

#include "Clip.h"
#include <memory>

namespace reaperlooper
{

/** Creates clips by type ID and deserializes a Clip's ValueTree back into the right subclass. */
namespace ClipFactory
{
    /** nullptr if typeId is unrecognised. */
    std::unique_ptr<Clip> createDefault (const juce::String& typeId);

    /** nullptr if the tree's "type" property is missing/unrecognised. */
    std::unique_ptr<Clip> fromValueTree (const juce::ValueTree& tree);
}

} // namespace reaperlooper
