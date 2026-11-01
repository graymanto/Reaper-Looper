#pragma once

/**
 * Minimal opaque-type forward declarations for reaper_plugin_functions.h,
 * used in place of the real reaper_plugin.h.
 *
 * The real reaper_plugin.h pulls in <windows.h> on Windows and WDL's SWELL
 * (plus pthread) on other platforms, because it defines the native
 * extension entry point machinery (ReaperPluginEntry, HINSTANCE, etc).
 * We don't need any of that: this project talks to REAPER as a VST3 plugin
 * that resolves API functions by name via IReaperHostApplication::getReaperApi
 * (see ReaperApi.h), not as a native extension loaded through
 * reaper_plugin_info_t.
 *
 * reaper_plugin_functions.h only ever uses these REAPER types as opaque
 * pointers (MediaTrack*, MediaItem*, ...), so forward declarations are all
 * that's needed to satisfy every function signature we select via
 * ReaperApiWantList.h. If a future REAPERAPI_WANT_* addition pulls in an
 * opaque type not listed here, add it here rather than including the real
 * reaper_plugin.h.
 */

class ReaProject;
class MediaTrack;
class MediaItem;
class MediaItem_Take;
class TrackEnvelope;
class PCM_source;

// Tells reaper_plugin_functions.h not to #include the real reaper_plugin.h
// (see the #ifndef _REAPER_PLUGIN_H_ guard near the top of that file).
#define _REAPER_PLUGIN_H_
