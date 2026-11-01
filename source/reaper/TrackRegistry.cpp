#include "TrackRegistry.h"

namespace reaperlooper::TrackRegistry
{

TrackInfo describeTrack (const ReaperFunctions& f, MediaTrack* track)
{
    TrackInfo info;
    info.track = track;

    if (track == nullptr)
        return info;

    if (f.getSetMediaTrackInfo_String != nullptr)
    {
        char name[256] = {};
        f.getSetMediaTrackInfo_String (track, "P_NAME", name, false);
        info.name = juce::String (name);
    }

    if (f.getSetMediaTrackInfo != nullptr && f.colorFromNative != nullptr)
    {
        // I_CUSTOMCOLOR is an int-valued field: GetSetMediaTrackInfo returns
        // a pointer *to* REAPER's internal int, which must be dereferenced
        // (unlike P_NAME-style char* fields, where the field itself already
        // is the pointer being returned). Bit 0x1000000 marks "colour is in
        // use"; without it the track has no custom colour.
        if (void* colourPtr = f.getSetMediaTrackInfo (track, "I_CUSTOMCOLOR", nullptr))
        {
            const int rawColour = *static_cast<int*> (colourPtr);
            if ((rawColour & 0x1000000) != 0)
            {
                int r = 0, g = 0, b = 0;
                f.colorFromNative (rawColour & 0xFFFFFF, &r, &g, &b);
                info.colourARGB = 0xFF000000u | (static_cast<juce::uint32> (r) << 16)
                                 | (static_cast<juce::uint32> (g) << 8) | static_cast<juce::uint32> (b);
            }
        }
    }

    return info;
}

std::vector<TrackInfo> listTracks (const ReaperFunctions& f, ReaProject* proj)
{
    std::vector<TrackInfo> result;

    if (f.countTracks == nullptr || f.getTrack == nullptr)
        return result;

    const int n = f.countTracks (proj);
    result.reserve (static_cast<size_t> (juce::jmax (0, n)));

    for (int i = 0; i < n; ++i)
        if (auto* track = f.getTrack (proj, i))
            result.push_back (describeTrack (f, track));

    return result;
}

MediaTrack* createTrack (const ReaperFunctions& f, ReaProject* proj, const juce::String& name)
{
    if (f.countTracks == nullptr || f.getTrack == nullptr || f.insertTrackAtIndex == nullptr
        || f.undo_beginBlock2 == nullptr || f.undo_endBlock2 == nullptr)
        return nullptr;

    f.undo_beginBlock2 (proj);

    // InsertTrackAtIndex takes no project parameter - like REAPER's CSurf_On*
    // transport functions, it always affects whichever project is currently
    // focused in the UI. If `proj` isn't the focused tab, the new track will
    // land at the computed index in a *different* project than intended.
    // This is a REAPER platform limitation, not something this adapter can
    // route around (see TransportBridge's equivalent note).
    const int newIndex = f.countTracks (proj);
    f.insertTrackAtIndex (newIndex, true);
    auto* track = f.getTrack (proj, newIndex);

    if (track != nullptr && name.isNotEmpty() && f.getSetMediaTrackInfo_String != nullptr)
        f.getSetMediaTrackInfo_String (track, "P_NAME", const_cast<char*> (name.toRawUTF8()), true);

    f.undo_endBlock2 (proj, "ReaperLooper: create track", 0);

    return track;
}

} // namespace reaperlooper::TrackRegistry
