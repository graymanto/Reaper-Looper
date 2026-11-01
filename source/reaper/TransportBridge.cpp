#include "TransportBridge.h"

namespace reaperlooper::TransportBridge
{

HostTransportState read (const ReaperFunctions& f, ReaProject* proj)
{
    HostTransportState state; // defaults: stopped, 120bpm, bar 0 / 4 QN long

    if (f.getPlayPosition2Ex == nullptr || f.getPlayStateEx == nullptr
        || f.timeMap2_timeToBeats == nullptr || f.timeMap_getMeasureInfo == nullptr
        || f.master_getTempo == nullptr)
        return state;

    const double posSeconds = f.getPlayPosition2Ex (proj);
    const int playState = f.getPlayStateEx (proj);
    state.isPlaying = (playState & 1) != 0;
    state.isRecording = (playState & 4) != 0;

    int measureIdx = 0;
    int cml = 0;
    int cdenom = 0;
    double fullbeats = 0.0;
    f.timeMap2_timeToBeats (proj, posSeconds, &measureIdx, &cml, &fullbeats, &cdenom);
    state.positionQuarterNotes = fullbeats;
    state.barIndex = measureIdx;

    double qnStart = 0.0;
    double qnEnd = 0.0;
    int tsNum = 0;
    int tsDenom = 0;
    double tempoAtMeasure = 0.0;
    f.timeMap_getMeasureInfo (proj, measureIdx, &qnStart, &qnEnd, &tsNum, &tsDenom, &tempoAtMeasure);
    state.barStartQuarterNotes = qnStart;
    // Guard against a degenerate zero-length measure rather than letting
    // downstream division-by-beatsPerSample-adjacent math blow up on it.
    state.barLengthQuarterNotes = (qnEnd > qnStart) ? (qnEnd - qnStart) : 4.0;

    state.bpm = f.master_getTempo();

    return state;
}

void play (const ReaperFunctions& f)
{
    if (f.csurf_onPlay != nullptr)
        f.csurf_onPlay();
}

void stop (const ReaperFunctions& f)
{
    if (f.csurf_onStop != nullptr)
        f.csurf_onStop();
}

void toggleRecord (const ReaperFunctions& f)
{
    if (f.csurf_onRecord != nullptr)
        f.csurf_onRecord();
}

} // namespace reaperlooper::TransportBridge
