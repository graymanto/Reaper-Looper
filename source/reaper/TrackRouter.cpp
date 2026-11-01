#include "TrackRouter.h"
#include <juce_core/juce_core.h>

namespace reaperlooper::TrackRouter
{

int findExistingSend (const ReaperFunctions& f, MediaTrack* sourceTrack, MediaTrack* destTrack)
{
    if (f.getTrackNumSends == nullptr || f.getSetTrackSendInfo == nullptr
        || sourceTrack == nullptr || destTrack == nullptr)
        return -1;

    const int n = f.getTrackNumSends (sourceTrack, 0); // category 0 = sends
    for (int i = 0; i < n; ++i)
    {
        // P_DESTTRACK is itself a pointer-valued field (MediaTrack*), so -
        // unlike I_CUSTOMCOLOR's int* dereference in TrackRegistry -
        // GetSetTrackSendInfo's returned void* is cast directly: the
        // "pointer to the field" and "the field's value" coincide when the
        // field is already a pointer.
        void* destPtr = f.getSetTrackSendInfo (sourceTrack, 0, i, "P_DESTTRACK", nullptr);
        if (static_cast<MediaTrack*> (destPtr) == destTrack)
            return i;
    }

    return -1;
}

int routeColumnToTrack (const ReaperFunctions& f, ReaProject* proj,
                        MediaTrack* sourceTrack, MediaTrack* destTrack, int midiChannel)
{
    if (sourceTrack == nullptr || destTrack == nullptr
        || f.createTrackSend == nullptr || f.setTrackSendInfo_Value == nullptr
        || f.undo_beginBlock2 == nullptr || f.undo_endBlock2 == nullptr)
        return -1;

    f.undo_beginBlock2 (proj);

    int sendIdx = findExistingSend (f, sourceTrack, destTrack);
    if (sendIdx < 0)
        sendIdx = f.createTrackSend (sourceTrack, destTrack);

    if (sendIdx >= 0)
    {
        // I_SRCCHAN = -1: no audio send. This plugin is MIDI-only
        // (IS_MIDI_EFFECT, no audio buses); CreateTrackSend's defaults
        // would otherwise carry audio too.
        f.setTrackSendInfo_Value (sourceTrack, 0, sendIdx, "I_SRCCHAN", -1.0);

        // I_MIDIFLAGS: low 5 bits = source channel (1-16, or 0 = all),
        // next 5 bits = destination channel (0 = orig, i.e. unchanged).
        // Filtering the source channel to this column's dedicated MIDI
        // channel is what makes "route column N onto this track" mean
        // only column N's notes arrive there, even though every column
        // shares one plugin instance's MIDI output.
        const double midiFlags = static_cast<double> (juce::jlimit (1, 16, midiChannel));
        f.setTrackSendInfo_Value (sourceTrack, 0, sendIdx, "I_MIDIFLAGS", midiFlags);
    }

    f.undo_endBlock2 (proj, "ReaperLooper: route column to track", 0);

    return sendIdx;
}

void unroute (const ReaperFunctions& f, ReaProject* proj, MediaTrack* sourceTrack, MediaTrack* destTrack)
{
    if (f.removeTrackSend == nullptr || f.undo_beginBlock2 == nullptr || f.undo_endBlock2 == nullptr)
        return;

    const int sendIdx = findExistingSend (f, sourceTrack, destTrack);
    if (sendIdx < 0)
        return;

    f.undo_beginBlock2 (proj);
    f.removeTrackSend (sourceTrack, 0, sendIdx);
    f.undo_endBlock2 (proj, "ReaperLooper: remove column routing", 0);
}

} // namespace reaperlooper::TrackRouter
