#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "ReaperFunctions.h"

// ---------------------------------------------------------------------------
// REAPER's IReaperHostApplication interface (a VST3 host-context extension)
// is how a plugin instance resolves the REAPER C/C++ extension API by name -
// the exact same function table a native REAPER extension gets via
// reaper_plugin_info_t.GetFunc(). See source/reaper/sdk/reaper_vst3_interfaces.h
// and the "VST3 plugins" note at the top of reaper_plugin_functions.h.
// ---------------------------------------------------------------------------
JUCE_BEGIN_IGNORE_WARNINGS_GCC_LIKE ("-Wzero-as-null-pointer-constant",
                                     "-Wunused-parameter",
                                     "-Wnon-virtual-dtor")

#include <pluginterfaces/base/ftypes.h>
#include <pluginterfaces/base/funknown.h>

namespace reaperlooper::reaper_sdk
{
    using namespace Steinberg;
    using uint32 = Steinberg::uint32;

    #include "sdk/reaper_vst3_interfaces.h"
}

JUCE_END_IGNORE_WARNINGS_GCC_LIKE

// ---------------------------------------------------------------------------
// The REAPER C/C++ extension API function table, declared at *global* scope
// with unqualified names (::GetTrack, ::GetNumTracks, ...) - that's how every
// piece of REAPER extension code refers to them, and keeping the convention
// makes this code recognisable to anyone who already knows the REAPER SDK.
//
// Which functions get declared/resolved is controlled entirely by
// ReaperApiWantList.h - add to that list, not here, when a new part of
// source/reaper/ needs another REAPER API function.
// ---------------------------------------------------------------------------
#include "sdk/ReaperForwardDecls.h"
#include "ReaperApiWantList.h"
#include "sdk/reaper_plugin_functions.h"

namespace reaperlooper
{

/**
 * Owns the connection to REAPER's C++ extension API from inside a VST3
 * plugin instance.
 *
 * Wired up via AudioProcessor::getVST3ClientExtensions() returning this
 * object. The JUCE VST3 wrapper calls setIHostApplication() shortly after
 * construction (message thread) with the host's IHostApplication; from
 * there we queryInterface for reaper::IReaperHostApplication and resolve
 * every function named in ReaperApiWantList.h via getReaperApi(name).
 *
 * Outside REAPER (Standalone, or a non-REAPER VST3 host), setIHostApplication
 * either isn't called or the queryInterface fails - isConnected() stays
 * false and every REAPER API function pointer stays null. Always check
 * isConnected() (or that a specific pointer is non-null) before calling into
 * the API; see reaper_plugin_functions.h's own note on this.
 *
 * Thread safety: setIHostApplication() runs once, early, on the message
 * thread. Once connected_ is true the resolved function pointers never
 * change again for the life of the plugin instance, so reading them from
 * the audio thread thereafter is safe without further synchronisation.
 *
 * One subtlety worth being explicit about: the resolved REAPER API function
 * pointers (declared at global scope by reaper_plugin_functions.h, and
 * mirrored into getFunctions()) are shared process-wide - every plugin
 * instance resolves to the same underlying REAPER engine functions, so
 * sharing them is correct and harmless. getHostTrack()/getHostProject() are
 * NOT shared, though: they come from *this instance's* IReaperHostApplication,
 * because each plugin instance sits on a different track. hostApp_ is
 * therefore a genuine per-instance member, not file-local state.
 */
class ReaperApi final : public juce::VST3ClientExtensions
{
public:
    ReaperApi() = default;
    ~ReaperApi() override = default;

    void setIHostApplication (Steinberg::FUnknown* hostContext) override;

    /** True once IReaperHostApplication was found and every wanted function resolved. */
    bool isConnected() const noexcept { return connected_; }

    /** Count of wanted functions REAPER failed to resolve (-1 = setIHostApplication never ran). */
    int numUnresolvedFunctions() const noexcept { return unresolvedCount_; }

    /** The resolved REAPER API, for use by TransportBridge/TrackRegistry/TrackRouter. Every member is null until isConnected(). */
    const ReaperFunctions& getFunctions() const noexcept { return functions_; }

    /** The MediaTrack this plugin instance is inserted on, or nullptr (not connected, or not running as track FX - e.g. on the master track). */
    MediaTrack* getHostTrack() const noexcept;

    /** The ReaProject this plugin instance belongs to, or nullptr if not connected. Pass this (not nullptr) to project-aware REAPER API calls so a multi-tab project doesn't silently affect the wrong one. */
    ReaProject* getHostProject() const noexcept;

private:
    bool connected_ { false };
    int unresolvedCount_ { -1 };
    ReaperFunctions functions_;
    reaper_sdk::IReaperHostApplication* hostApp_ { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReaperApi)
};

} // namespace reaperlooper
