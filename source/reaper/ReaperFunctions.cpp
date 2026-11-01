#include "ReaperFunctions.h"
#include "ReaperApiWantList.h"
#include "sdk/reaper_plugin_functions.h"

namespace reaperlooper
{

ReaperFunctions ReaperFunctions::fromGlobalApi()
{
    ReaperFunctions f;

    f.getPlayPosition2Ex = ::GetPlayPosition2Ex;
    f.getPlayStateEx = ::GetPlayStateEx;
    f.timeMap2_timeToBeats = ::TimeMap2_timeToBeats;
    f.timeMap_getMeasureInfo = ::TimeMap_GetMeasureInfo;
    f.master_getTempo = ::Master_GetTempo;
    f.csurf_onPlay = ::CSurf_OnPlay;
    f.csurf_onStop = ::CSurf_OnStop;
    f.csurf_onRecord = ::CSurf_OnRecord;

    f.countTracks = ::CountTracks;
    f.getTrack = ::GetTrack;
    f.getSetMediaTrackInfo_String = ::GetSetMediaTrackInfo_String;
    f.getSetMediaTrackInfo = ::GetSetMediaTrackInfo;
    f.insertTrackAtIndex = ::InsertTrackAtIndex;
    f.colorFromNative = ::ColorFromNative;
    f.colorToNative = ::ColorToNative;

    f.createTrackSend = ::CreateTrackSend;
    f.removeTrackSend = ::RemoveTrackSend;
    f.getTrackNumSends = ::GetTrackNumSends;
    f.getSetTrackSendInfo = ::GetSetTrackSendInfo;
    f.setTrackSendInfo_Value = ::SetTrackSendInfo_Value;

    f.undo_beginBlock2 = ::Undo_BeginBlock2;
    f.undo_endBlock2 = ::Undo_EndBlock2;

    return f;
}

} // namespace reaperlooper
