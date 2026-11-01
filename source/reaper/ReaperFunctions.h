#pragma once

#include "sdk/ReaperForwardDecls.h"

namespace reaperlooper
{

/**
 * A plain struct of REAPER API function pointers, matching exactly the
 * subset resolved via ReaperApiWantList.h.
 *
 * This is the seam that makes the rest of the adapter layer
 * (TransportBridge, TrackRegistry, TrackRouter) testable without a running
 * REAPER instance: production code gets a populated one from
 * ReaperApi::getFunctions() (itself filled in by fromGlobalApi() once
 * connected), while tests construct one directly and point its members at
 * fake implementations. None of those adapter namespaces call the global
 * ::FunctionName symbols directly - they only ever go through a
 * ReaperFunctions reference, matching the plan's verification approach.
 *
 * Every member defaults to nullptr; every adapter function must check
 * before calling (a default-constructed ReaperFunctions{} is exactly what
 * "not connected to REAPER" looks like).
 */
struct ReaperFunctions
{
    //==============================================================================
    // Transport / tempo map

    double (*getPlayPosition2Ex) (ReaProject*) = nullptr;
    int (*getPlayStateEx) (ReaProject*) = nullptr;
    double (*timeMap2_timeToBeats) (ReaProject*, double, int*, int*, double*, int*) = nullptr;
    double (*timeMap_getMeasureInfo) (ReaProject*, int, double*, double*, int*, int*, double*) = nullptr;
    double (*master_getTempo)() = nullptr;
    void (*csurf_onPlay)() = nullptr;
    void (*csurf_onStop)() = nullptr;
    void (*csurf_onRecord)() = nullptr;

    //==============================================================================
    // Tracks

    int (*countTracks) (ReaProject*) = nullptr;
    MediaTrack* (*getTrack) (ReaProject*, int) = nullptr;
    bool (*getSetMediaTrackInfo_String) (MediaTrack*, const char*, char*, bool) = nullptr;
    void* (*getSetMediaTrackInfo) (MediaTrack*, const char*, void*) = nullptr;
    void (*insertTrackAtIndex) (int, bool) = nullptr;
    void (*colorFromNative) (int, int*, int*, int*) = nullptr;
    int (*colorToNative) (int, int, int) = nullptr;

    //==============================================================================
    // Routing (sends)

    int (*createTrackSend) (MediaTrack*, MediaTrack*) = nullptr;
    bool (*removeTrackSend) (MediaTrack*, int, int) = nullptr;
    int (*getTrackNumSends) (MediaTrack*, int) = nullptr;
    void* (*getSetTrackSendInfo) (MediaTrack*, int, int, const char*, void*) = nullptr;
    bool (*setTrackSendInfo_Value) (MediaTrack*, int, int, const char*, double) = nullptr;

    //==============================================================================
    // Undo

    void (*undo_beginBlock2) (ReaProject*) = nullptr;
    void (*undo_endBlock2) (ReaProject*, const char*, int) = nullptr;

    //==============================================================================
    /** Production use: populates every member from the resolved global REAPER API symbols. */
    static ReaperFunctions fromGlobalApi();
};

} // namespace reaperlooper
