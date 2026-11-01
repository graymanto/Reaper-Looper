#include <catch2/catch_all.hpp>
#include "reaper/TransportBridge.h"

using namespace reaperlooper;

namespace
{
    // Simple test-local fake transport state, independent of FakeReaper.h's
    // track/send fakes (nothing here needs a fake project at all).
    struct FakeTransport
    {
        double posSeconds { 0.0 };
        int playState { 0 }; // &1 playing, &4 recording
        double bpm { 120.0 };
        int measureIdx { 0 };
        double fullBeats { 0.0 };
        double qnStart { 0.0 };
        double qnEnd { 4.0 };
        int tsNum { 4 };
        int tsDenom { 4 };

        int playCalls { 0 };
        int stopCalls { 0 };
        int recordCalls { 0 };
    };

    FakeTransport& fakeTransport() { static FakeTransport t; return t; }
    void resetFakeTransport() { fakeTransport() = FakeTransport(); }

    double fakeGetPlayPosition2Ex (ReaProject*) { return fakeTransport().posSeconds; }
    int fakeGetPlayStateEx (ReaProject*) { return fakeTransport().playState; }

    double fakeTimeMap2_timeToBeats (ReaProject*, double, int* measuresOut, int* cmlOut, double* fullbeatsOut, int* cdenomOut)
    {
        auto& t = fakeTransport();
        if (measuresOut != nullptr) *measuresOut = t.measureIdx;
        if (cmlOut != nullptr) *cmlOut = t.tsNum;
        if (fullbeatsOut != nullptr) *fullbeatsOut = t.fullBeats;
        if (cdenomOut != nullptr) *cdenomOut = t.tsDenom;
        return 0.0; // "beats since measure start" - not used by TransportBridge::read
    }

    double fakeTimeMap_GetMeasureInfo (ReaProject*, int, double* qnStartOut, double* qnEndOut,
                                        int* tsNumOut, int* tsDenomOut, double* tempoOut)
    {
        auto& t = fakeTransport();
        if (qnStartOut != nullptr) *qnStartOut = t.qnStart;
        if (qnEndOut != nullptr) *qnEndOut = t.qnEnd;
        if (tsNumOut != nullptr) *tsNumOut = t.tsNum;
        if (tsDenomOut != nullptr) *tsDenomOut = t.tsDenom;
        if (tempoOut != nullptr) *tempoOut = t.bpm;
        return 0.0; // measure start time in seconds - not used by TransportBridge::read
    }

    double fakeMaster_GetTempo() { return fakeTransport().bpm; }
    void fakeCsurf_OnPlay() { ++fakeTransport().playCalls; }
    void fakeCsurf_OnStop() { ++fakeTransport().stopCalls; }
    void fakeCsurf_OnRecord() { ++fakeTransport().recordCalls; }

    ReaperFunctions makeFakeFunctions()
    {
        ReaperFunctions f;
        f.getPlayPosition2Ex = fakeGetPlayPosition2Ex;
        f.getPlayStateEx = fakeGetPlayStateEx;
        f.timeMap2_timeToBeats = fakeTimeMap2_timeToBeats;
        f.timeMap_getMeasureInfo = fakeTimeMap_GetMeasureInfo;
        f.master_getTempo = fakeMaster_GetTempo;
        f.csurf_onPlay = fakeCsurf_OnPlay;
        f.csurf_onStop = fakeCsurf_OnStop;
        f.csurf_onRecord = fakeCsurf_OnRecord;
        return f;
    }
}

TEST_CASE ("TransportBridge::read returns a safe default when not connected", "[reaper][transport]")
{
    const ReaperFunctions empty;
    const auto state = TransportBridge::read (empty, nullptr);

    REQUIRE_FALSE (state.isPlaying);
    REQUIRE_FALSE (state.isRecording);
    REQUIRE (state.bpm == Catch::Approx (120.0));
    REQUIRE (state.barIndex == 0);
}

TEST_CASE ("TransportBridge::read reflects REAPER's live transport and tempo map", "[reaper][transport]")
{
    resetFakeTransport();
    auto& t = fakeTransport();
    t.playState = 1; // playing, not recording
    t.bpm = 140.0;
    t.measureIdx = 3;
    t.fullBeats = 12.5;
    t.qnStart = 12.0;
    t.qnEnd = 16.0;

    const auto state = TransportBridge::read (makeFakeFunctions(), nullptr);

    REQUIRE (state.isPlaying);
    REQUIRE_FALSE (state.isRecording);
    REQUIRE (state.bpm == Catch::Approx (140.0));
    REQUIRE (state.barIndex == 3);
    REQUIRE (state.positionQuarterNotes == Catch::Approx (12.5));
    REQUIRE (state.barStartQuarterNotes == Catch::Approx (12.0));
    REQUIRE (state.barLengthQuarterNotes == Catch::Approx (4.0));
}

TEST_CASE ("TransportBridge::read reports recording", "[reaper][transport]")
{
    resetFakeTransport();
    fakeTransport().playState = 1 | 4;

    const auto state = TransportBridge::read (makeFakeFunctions(), nullptr);
    REQUIRE (state.isPlaying);
    REQUIRE (state.isRecording);
}

TEST_CASE ("TransportBridge::read falls back to a safe bar length for a degenerate zero-length measure", "[reaper][transport]")
{
    resetFakeTransport();
    auto& t = fakeTransport();
    t.qnStart = 10.0;
    t.qnEnd = 10.0; // qnEnd <= qnStart - should never happen in a real project, but mustn't divide-by-zero downstream

    const auto state = TransportBridge::read (makeFakeFunctions(), nullptr);
    REQUIRE (state.barLengthQuarterNotes == Catch::Approx (4.0));
}

TEST_CASE ("TransportBridge play/stop/toggleRecord call through to CSurf_On*", "[reaper][transport]")
{
    resetFakeTransport();
    const auto f = makeFakeFunctions();

    TransportBridge::play (f);
    TransportBridge::stop (f);
    TransportBridge::toggleRecord (f);

    REQUIRE (fakeTransport().playCalls == 1);
    REQUIRE (fakeTransport().stopCalls == 1);
    REQUIRE (fakeTransport().recordCalls == 1);
}

TEST_CASE ("TransportBridge play/stop/toggleRecord are safe no-ops when not connected", "[reaper][transport]")
{
    const ReaperFunctions empty;
    TransportBridge::play (empty);
    TransportBridge::stop (empty);
    TransportBridge::toggleRecord (empty);
    SUCCEED ("no crash");
}
