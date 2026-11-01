#pragma once

#include "reaper/ReaperFunctions.h"
#include <juce_core/juce_core.h>
#include <memory>
#include <vector>

/**
 * A minimal fake REAPER project, used to unit-test TrackRegistry and
 * TrackRouter without a running REAPER instance - per the plan's
 * verification approach ("inject a mock REAPER function-pointer table").
 *
 * MediaTrack/ReaProject are opaque, never-defined types everywhere in
 * production code (including here) - never dereferenced as themselves,
 * only ever passed around as handles. That makes it safe to hand out
 * `reinterpret_cast`ed pointers to our own FakeTrack struct as MediaTrack*:
 * nothing anywhere ever casts them back to any OTHER type, so there is no
 * aliasing violation in practice, only in principle. toHandle()/fromHandle()
 * are the only places the cast happens, keeping that assumption contained.
 */
namespace FakeReaper
{

struct FakeSend
{
    MediaTrack* dest { nullptr };
    double srcChan { 0.0 };   ///< I_SRCCHAN as last set via setTrackSendInfo_Value.
    int midiFlags { 31 };     ///< I_MIDIFLAGS; 31 = "MIDI send disabled", REAPER's real default for a fresh send.
};

struct FakeTrack
{
    juce::String name;
    int customColour { 0 };   ///< Raw I_CUSTOMCOLOR value; 0 = unset.
    std::vector<FakeSend> sends;
};

struct World
{
    std::vector<std::unique_ptr<FakeTrack>> tracks;
    int undoBeginCount { 0 };
    int undoEndCount { 0 };
    juce::String lastUndoDescription;

    FakeTrack* addTrack (juce::String name = {})
    {
        tracks.push_back (std::make_unique<FakeTrack>());
        tracks.back()->name = std::move (name);
        return tracks.back().get();
    }
};

/** Test-local fake REAPER state - call reset() at the top of every TEST_CASE that uses it. */
inline World& world() { static World w; return w; }
inline void reset() { world() = World(); }

inline MediaTrack* toHandle (FakeTrack* t) { return reinterpret_cast<MediaTrack*> (t); }
inline FakeTrack* fromHandle (MediaTrack* t) { return reinterpret_cast<FakeTrack*> (t); }

//==============================================================================
// Fakes matching ReaperFunctions' member signatures exactly.

inline int countTracks (ReaProject*) { return static_cast<int> (world().tracks.size()); }

inline MediaTrack* getTrack (ReaProject*, int idx)
{
    auto& w = world();
    return (idx >= 0 && idx < static_cast<int> (w.tracks.size()))
        ? toHandle (w.tracks[static_cast<size_t> (idx)].get())
        : nullptr;
}

inline bool getSetMediaTrackInfo_String (MediaTrack* tr, const char* parm, char* buf, bool set)
{
    auto* t = fromHandle (tr);
    if (t == nullptr || juce::String (parm) != "P_NAME")
        return false;

    if (set)
        t->name = juce::String (buf);
    else
        t->name.copyToUTF8 (buf, 256);

    return true;
}

inline void* getSetMediaTrackInfo (MediaTrack* tr, const char* parm, void* setNewValue)
{
    auto* t = fromHandle (tr);
    if (t == nullptr || juce::String (parm) != "I_CUSTOMCOLOR")
        return nullptr;

    if (setNewValue != nullptr)
        t->customColour = *static_cast<int*> (setNewValue);

    return &t->customColour;
}

inline void insertTrackAtIndex (int idx, bool /*wantDefaults*/)
{
    auto& w = world();
    const auto clamped = juce::jlimit (0, static_cast<int> (w.tracks.size()), idx);
    w.tracks.insert (w.tracks.begin() + clamped, std::make_unique<FakeTrack>());
}

inline void colorFromNative (int col, int* r, int* g, int* b)
{
    if (r != nullptr) *r = (col >> 16) & 0xFF;
    if (g != nullptr) *g = (col >> 8) & 0xFF;
    if (b != nullptr) *b = col & 0xFF;
}

inline int colorToNative (int r, int g, int b) { return (r << 16) | (g << 8) | b; }

inline int getTrackNumSends (MediaTrack* tr, int category)
{
    auto* t = fromHandle (tr);
    return (t != nullptr && category == 0) ? static_cast<int> (t->sends.size()) : 0;
}

inline void* getSetTrackSendInfo (MediaTrack* tr, int category, int idx, const char* parm, void* setNewValue)
{
    auto* t = fromHandle (tr);
    if (t == nullptr || category != 0 || idx < 0 || idx >= static_cast<int> (t->sends.size()))
        return nullptr;

    if (juce::String (parm) == "P_DESTTRACK" && setNewValue == nullptr) // read-only in practice
        return t->sends[static_cast<size_t> (idx)].dest;

    return nullptr;
}

inline bool setTrackSendInfo_Value (MediaTrack* tr, int category, int idx, const char* parm, double value)
{
    auto* t = fromHandle (tr);
    if (t == nullptr || category != 0 || idx < 0 || idx >= static_cast<int> (t->sends.size()))
        return false;

    const juce::String p (parm);
    auto& send = t->sends[static_cast<size_t> (idx)];
    if (p == "I_SRCCHAN") { send.srcChan = value; return true; }
    if (p == "I_MIDIFLAGS") { send.midiFlags = static_cast<int> (value); return true; }
    return false;
}

inline int createTrackSend (MediaTrack* src, MediaTrack* dest)
{
    auto* t = fromHandle (src);
    if (t == nullptr)
        return -1;
    t->sends.push_back ({ dest, 0.0, 31 });
    return static_cast<int> (t->sends.size()) - 1;
}

inline bool removeTrackSend (MediaTrack* tr, int category, int idx)
{
    auto* t = fromHandle (tr);
    if (t == nullptr || category != 0 || idx < 0 || idx >= static_cast<int> (t->sends.size()))
        return false;
    t->sends.erase (t->sends.begin() + idx);
    return true;
}

inline void undo_beginBlock2 (ReaProject*) { ++world().undoBeginCount; }

inline void undo_endBlock2 (ReaProject*, const char* desc, int)
{
    ++world().undoEndCount;
    world().lastUndoDescription = juce::String (desc);
}

inline reaperlooper::ReaperFunctions makeFunctions()
{
    reaperlooper::ReaperFunctions f;
    f.countTracks = countTracks;
    f.getTrack = getTrack;
    f.getSetMediaTrackInfo_String = getSetMediaTrackInfo_String;
    f.getSetMediaTrackInfo = getSetMediaTrackInfo;
    f.insertTrackAtIndex = insertTrackAtIndex;
    f.colorFromNative = colorFromNative;
    f.colorToNative = colorToNative;
    f.getTrackNumSends = getTrackNumSends;
    f.getSetTrackSendInfo = getSetTrackSendInfo;
    f.setTrackSendInfo_Value = setTrackSendInfo_Value;
    f.createTrackSend = createTrackSend;
    f.removeTrackSend = removeTrackSend;
    f.undo_beginBlock2 = undo_beginBlock2;
    f.undo_endBlock2 = undo_endBlock2;
    return f;
}

} // namespace FakeReaper
