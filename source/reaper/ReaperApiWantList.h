#pragma once

/**
 * Single source of truth for which REAPER API functions this plugin resolves.
 *
 * reaper_plugin_functions.h declares every REAPER API function ever
 * exported, gated per-function behind
 *   #if defined(REAPERAPI_WANT_<Name>) || !defined(REAPERAPI_MINIMAL)
 * With REAPERAPI_MINIMAL defined, only the functions named below are
 * declared/loaded. Add a line here (and to ReaperFunctions::fromGlobalApi(),
 * which reads these same global symbols into the testable ReaperFunctions
 * struct) whenever a new part of the adapter layer (source/reaper/) needs
 * another function.
 *
 * Keep this list sorted alphabetically to make additions easy to diff.
 */

#define REAPERAPI_MINIMAL
#define REAPERAPI_NO_LICE

#define REAPERAPI_WANT_ColorFromNative
#define REAPERAPI_WANT_ColorToNative
#define REAPERAPI_WANT_CountTracks
#define REAPERAPI_WANT_CreateNewMIDIItemInProj
#define REAPERAPI_WANT_CreateTrackSend
#define REAPERAPI_WANT_CSurf_OnPlay
#define REAPERAPI_WANT_CSurf_OnRecord
#define REAPERAPI_WANT_CSurf_OnStop
#define REAPERAPI_WANT_GetActiveTake
#define REAPERAPI_WANT_GetPlayPosition2Ex
#define REAPERAPI_WANT_GetPlayStateEx
#define REAPERAPI_WANT_GetSetMediaTrackInfo
#define REAPERAPI_WANT_GetSetMediaTrackInfo_String
#define REAPERAPI_WANT_GetSetTrackSendInfo
#define REAPERAPI_WANT_GetTrack
#define REAPERAPI_WANT_GetTrackNumSends
#define REAPERAPI_WANT_InsertTrackAtIndex
#define REAPERAPI_WANT_Master_GetTempo
#define REAPERAPI_WANT_MIDI_InsertNote
#define REAPERAPI_WANT_RemoveTrackSend
#define REAPERAPI_WANT_SetTrackSendInfo_Value
#define REAPERAPI_WANT_TimeMap2_timeToBeats
#define REAPERAPI_WANT_TimeMap2_timeToQN
#define REAPERAPI_WANT_TimeMap_GetMeasureInfo
#define REAPERAPI_WANT_TimeMap_QNToTime
#define REAPERAPI_WANT_Undo_BeginBlock2
#define REAPERAPI_WANT_Undo_EndBlock2
