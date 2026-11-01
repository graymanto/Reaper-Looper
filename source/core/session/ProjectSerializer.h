#pragma once

#include "Session.h"

namespace reaperlooper
{

/**
 * Thin, validating wrapper around Session::toValueTree()/fromValueTree().
 * File I/O (or, in production, storing the tree in a REAPER project - see
 * the plan's note on this) is the caller's responsibility; this class only
 * handles the XML/ValueTree <-> Session conversion and version-checking.
 * Mirrors `sequenced`'s ProjectSerializer.
 */
namespace ProjectSerializer
{
    juce::String sessionToXml (const Session& session);
    bool sessionFromXml (Session& session, const juce::String& xml, juce::String* errorMessage = nullptr);

    juce::ValueTree sessionToValueTree (const Session& session);
    bool sessionFromValueTree (Session& session, const juce::ValueTree& tree, juce::String* errorMessage = nullptr);

    bool validateValueTree (const juce::ValueTree& tree, juce::String* errorMessage = nullptr);
}

} // namespace reaperlooper
