#include "Track.h"
#include "../model/ClipFactory.h"
#include "../engine/LaunchQuantizer.h"
#include "../engine/LoopSegmenter.h"
#include <algorithm>
#include <cmath>

namespace reaperlooper
{

namespace
{
    constexpr double kEpsilon = 1.0e-9;
}

//==============================================================================
// Grid management

void Track::ensureRowCount (int rows)
{
    if (rows > static_cast<int> (cells_.size()))
        cells_.resize (static_cast<size_t> (rows));
}

Clip* Track::getClip (int row) noexcept
{
    return (row >= 0 && row < static_cast<int> (cells_.size())) ? cells_[static_cast<size_t> (row)].get() : nullptr;
}

const Clip* Track::getClip (int row) const noexcept
{
    return (row >= 0 && row < static_cast<int> (cells_.size())) ? cells_[static_cast<size_t> (row)].get() : nullptr;
}

void Track::setClip (int row, std::unique_ptr<Clip> clip)
{
    ensureRowCount (row + 1);
    cells_[static_cast<size_t> (row)] = std::move (clip);
}

void Track::clearClip (int row)
{
    if (row < 0 || row >= static_cast<int> (cells_.size()))
        return;

    if (row == activeSlot_)
    {
        // See the header note: no MidiBuffer is available here to flush any
        // sounding notes. The caller is responsible for stopping the slot
        // first (handlePadDown / queueStop, resolved via processBlock)
        // before clearing it from the audio thread.
        activeSlot_ = -1;
        followTargetBarIndex_ = -1;
    }
    if (row == queuedSlot_)
    {
        queuedSlot_ = -1;
        hasQueuedBoundary_ = stopQueued_; // keep a pending stop if that's what was queued
    }

    cells_[static_cast<size_t> (row)].reset();
}

//==============================================================================
// Quantize resolution

LaunchQuantize Track::effectiveQuantize (const Clip* clip) const noexcept
{
    if (clip != nullptr)
        if (auto q = clip->getQuantizeOverride())
            return *q;
    return defaultQuantize_;
}

void Track::queueLaunch (int row, const HostTransportState& transport, bool bypassQuantize)
{
    const auto quantize = bypassQuantize ? LaunchQuantize::None : effectiveQuantize (getClip (row));
    queuedSlot_ = row;
    stopQueued_ = false;
    queuedBoundaryBeat_ = LaunchQuantizer::nextBoundary (transport, quantize);
    hasQueuedBoundary_ = true;
}

void Track::queueStop (const HostTransportState& transport, bool bypassQuantize)
{
    const auto quantize = bypassQuantize ? LaunchQuantize::None
                                          : effectiveQuantize (activeSlot_ >= 0 ? getClip (activeSlot_) : nullptr);
    queuedSlot_ = -1;
    stopQueued_ = true;
    queuedBoundaryBeat_ = LaunchQuantizer::nextBoundary (transport, quantize);
    hasQueuedBoundary_ = true;
}

//==============================================================================
// Pad / scene interaction

void Track::handlePadDown (int row, const HostTransportState& transport, juce::MidiBuffer& immediateOutput)
{
    if (! hasClip (row))
    {
        if (activeSlot_ >= 0 || queuedSlot_ >= 0)
            queueStop (transport, false);
        return;
    }

    auto* clip = getClip (row);

    switch (clip->getLaunchMode())
    {
        case LaunchMode::Gate:
            hasQueuedBoundary_ = false;
            resolveLaunchNow (row, transport.positionQuarterNotes, transport, &immediateOutput, 0);
            return;

        case LaunchMode::Repeat:
            repeating_ = true;
            queueLaunch (row, transport, false);
            return;

        case LaunchMode::Trigger:
            queueLaunch (row, transport, false);
            return;

        case LaunchMode::Toggle:
        default:
            if (row == queuedSlot_ && ! stopQueued_)
            {
                // Re-pressing a still-queued (not yet active) slot cancels the launch.
                hasQueuedBoundary_ = false;
                queuedSlot_ = -1;
                return;
            }
            if (row == activeSlot_)
            {
                queueStop (transport, false);
                return;
            }
            queueLaunch (row, transport, false);
            return;
    }
}

void Track::handlePadUp (int row, const HostTransportState& transport, juce::MidiBuffer& immediateOutput)
{
    juce::ignoreUnused (transport);

    auto* clip = getClip (row);
    if (clip == nullptr)
        return;

    if (clip->getLaunchMode() == LaunchMode::Gate && row == activeSlot_)
        resolveStopNow (&immediateOutput, 0);
    else if (clip->getLaunchMode() == LaunchMode::Repeat)
        repeating_ = false;
}

void Track::handlePadDownImmediate (int row, const HostTransportState& transport, juce::MidiBuffer& immediateOutput)
{
    if (! hasClip (row))
        return;

    hasQueuedBoundary_ = false;
    resolveLaunchNow (row, transport.positionQuarterNotes, transport, &immediateOutput, 0);
}

void Track::handleSceneLaunchForRow (int row, const HostTransportState& transport)
{
    if (! hasClip (row))
    {
        if (activeSlot_ >= 0 || queuedSlot_ >= 0)
            queueStop (transport, false);
        return;
    }

    // Deliberately does not special-case row == activeSlot_: scene launch
    // always (re)triggers, matching Ableton - pressing a scene restarts every
    // clip in it, even ones already playing.
    queueLaunch (row, transport, false);
}

void Track::queueStopAll (const HostTransportState& transport)
{
    if (activeSlot_ >= 0 || queuedSlot_ >= 0)
        queueStop (transport, false);
}

//==============================================================================
// Resolution

void Track::resolveQueuedBoundaryNow (const HostTransportState& transport, double resolvedBeat,
                                       juce::MidiBuffer& output, int sampleOffset)
{
    hasQueuedBoundary_ = false;

    if (stopQueued_)
    {
        resolveStopNow (&output, sampleOffset);
        return;
    }

    if (queuedSlot_ >= 0)
        resolveLaunchNow (queuedSlot_, resolvedBeat, transport, &output, sampleOffset);
}

void Track::resolveLaunchNow (int row, double resolvedBeat, const HostTransportState& transport,
                               juce::MidiBuffer* output, int sampleOffset)
{
    auto* newClip = getClip (row);
    if (newClip == nullptr)
        return;

    const bool sameClipRelaunch = (row == activeSlot_);
    double newAnchor = resolvedBeat;

    if (legatoMode_ && ! sameClipRelaunch && activeSlot_ >= 0)
    {
        if (auto* oldClip = getClip (activeSlot_))
        {
            const double oldLen = oldClip->getLengthBeats();
            const double oldPhase = oldLen > 0.0 ? std::fmod (resolvedBeat - activeClipLoopAnchorBeat_, oldLen) : 0.0;
            newAnchor = resolvedBeat - oldPhase;
        }
    }

    if (activeSlot_ >= 0 && output != nullptr)
        emitAllNotesOff (*output, sampleOffset); // covers both "different clip takes over" and "relaunch cuts ringing notes"

    activeSlot_ = row;
    queuedSlot_ = -1;
    stopQueued_ = false;
    activeClipLoopAnchorBeat_ = newAnchor;

    // Arm the follow action now, using the bar this activation actually
    // happened in. Arming lazily on the *next* processBlock call instead
    // would be off by one block: checkFollowAction() runs at the top of
    // processBlock, before this resolution - on the very call where a clip
    // becomes active, activeSlot_ is still the *previous* value at that
    // point, so the check would skip to the following block before arming,
    // silently adding up to one block's worth of delay to afterBars.
    followTargetBarIndex_ = (newClip->getFollowAction().type != FollowActionType::None)
        ? transport.barIndex + juce::jmax (1, newClip->getFollowAction().afterBars)
        : -1;

    if (newClip->getLaunchMode() == LaunchMode::Repeat && repeating_)
    {
        const auto quantize = effectiveQuantize (newClip);
        HostTransportState projected = transport;
        projected.positionQuarterNotes = resolvedBeat;

        queuedSlot_ = row;
        stopQueued_ = false;
        queuedBoundaryBeat_ = LaunchQuantizer::nextBoundary (projected, quantize);
        if (queuedBoundaryBeat_ <= resolvedBeat + kEpsilon)
            queuedBoundaryBeat_ = resolvedBeat + 0.25; // degenerate (None quantize): fall back to a quarter-note retrigger rate
        hasQueuedBoundary_ = true;
    }
}

void Track::resolveStopNow (juce::MidiBuffer* output, int sampleOffset)
{
    if (activeSlot_ >= 0 && output != nullptr)
        emitAllNotesOff (*output, sampleOffset);

    activeSlot_ = -1;
    queuedSlot_ = -1;
    stopQueued_ = false;
    hasQueuedBoundary_ = false;
    followTargetBarIndex_ = -1;
    repeating_ = false;
}

void Track::emitAllNotesOff (juce::MidiBuffer& output, int sampleOffset) const
{
    for (int ch = 1; ch <= 16; ++ch)
    {
        output.addEvent (juce::MidiMessage::allNotesOff (ch), sampleOffset);
        output.addEvent (juce::MidiMessage::allSoundOff (ch), sampleOffset);
    }
}

//==============================================================================
// Follow actions

int Track::resolveFollowTarget (const FollowAction& fa) const
{
    std::vector<int> occupied;
    for (int i = 0; i < getNumRows(); ++i)
        if (hasClip (i))
            occupied.push_back (i);

    if (occupied.empty())
        return -1;

    const auto it = std::find (occupied.begin(), occupied.end(), activeSlot_);
    const int activeIdx = (it != occupied.end()) ? static_cast<int> (std::distance (occupied.begin(), it)) : -1;

    switch (fa.type)
    {
        case FollowActionType::Same:
            return activeSlot_;

        case FollowActionType::Next:
            if (activeIdx < 0) return occupied.front();
            return occupied[static_cast<size_t> ((activeIdx + 1) % static_cast<int> (occupied.size()))];

        case FollowActionType::Previous:
            if (activeIdx < 0) return occupied.front();
            return occupied[static_cast<size_t> ((activeIdx - 1 + static_cast<int> (occupied.size())) % static_cast<int> (occupied.size()))];

        case FollowActionType::First:
            return occupied.front();

        case FollowActionType::Last:
            return occupied.back();

        case FollowActionType::Any:
            return occupied[static_cast<size_t> (juce::Random::getSystemRandom().nextInt (static_cast<int> (occupied.size())))];

        case FollowActionType::Other:
        {
            if (occupied.size() <= 1)
                return activeSlot_; // nothing else to choose - falls back to Same.
            std::vector<int> others;
            for (int r : occupied)
                if (r != activeSlot_)
                    others.push_back (r);
            return others[static_cast<size_t> (juce::Random::getSystemRandom().nextInt (static_cast<int> (others.size())))];
        }

        case FollowActionType::Specific:
            return hasClip (fa.targetRow) ? fa.targetRow : -1;

        case FollowActionType::None:
        default:
            return -1;
    }
}

void Track::checkFollowAction (const HostTransportState& transport, juce::MidiBuffer& output)
{
    if (activeSlot_ < 0)
    {
        followTargetBarIndex_ = -1;
        return;
    }

    auto* clip = getClip (activeSlot_);
    if (clip == nullptr || clip->getFollowAction().type == FollowActionType::None)
    {
        followTargetBarIndex_ = -1;
        return;
    }

    if (followTargetBarIndex_ < 0)
    {
        followTargetBarIndex_ = transport.barIndex + juce::jmax (1, clip->getFollowAction().afterBars);
        return;
    }

    if (transport.barIndex < followTargetBarIndex_)
        return;

    // Fires at this block's start rather than hunting for the exact sample
    // the target bar began on - at most one block's worth of early/late,
    // which is inaudible for a bar-counted event. See the header for why
    // launch/stop boundaries (which ARE sample-accurate) don't take this
    // shortcut.
    bool fired = false;
    if (juce::Random::getSystemRandom().nextFloat() <= clip->getFollowAction().chance)
    {
        const int target = resolveFollowTarget (clip->getFollowAction());
        if (target >= 0)
        {
            resolveLaunchNow (target, transport.positionQuarterNotes, transport, &output, 0);
            fired = true;
        }
    }

    followTargetBarIndex_ = fired ? -1 : (transport.barIndex + juce::jmax (1, clip->getFollowAction().afterBars));
}

//==============================================================================
// Processing

void Track::collectActiveClip (juce::MidiBuffer& output, int sampleOffset, int numSamplesForSegment,
                                double segmentStartBeat, double beatsPerSample)
{
    if (activeSlot_ < 0 || muted_ || numSamplesForSegment <= 0)
        return;

    auto* clip = getClip (activeSlot_);
    if (clip == nullptr)
        return;

    auto segments = LoopSegmenter::splitIntoLoopSegments (segmentStartBeat, numSamplesForSegment, beatsPerSample,
                                                            activeClipLoopAnchorBeat_, clip->getLengthBeats(),
                                                            clip->isLooping());

    int samplesCovered = 0;
    for (auto ctx : segments)
    {
        ctx.sampleOffset += sampleOffset;
        clip->collectMidi (output, ctx);
        samplesCovered += ctx.numSamples;
    }

    if (! clip->isLooping() && samplesCovered < numSamplesForSegment)
        resolveStopNow (&output, sampleOffset + samplesCovered); // one-shot clip reached its end mid-segment.
}

void Track::processBlock (juce::MidiBuffer& output, int numSamples, const HostTransportState& transport)
{
    if (sampleRate_ <= 0.0 || numSamples <= 0 || ! transport.isPlaying)
        return;

    const double beatsPerSample = (transport.bpm / 60.0) / sampleRate_;
    const double blockStartBeat = transport.positionQuarterNotes;
    const double blockEndBeat = blockStartBeat + static_cast<double> (numSamples) * beatsPerSample;

    checkFollowAction (transport, output);

    int splitSample = numSamples;
    bool hasSplit = false;

    if (hasQueuedBoundary_ && queuedBoundaryBeat_ >= blockStartBeat - kEpsilon && queuedBoundaryBeat_ < blockEndBeat)
    {
        splitSample = juce::jlimit (0, numSamples,
                                     static_cast<int> (std::round ((queuedBoundaryBeat_ - blockStartBeat) / beatsPerSample)));
        hasSplit = true;
    }

    if (splitSample > 0)
        collectActiveClip (output, 0, splitSample, blockStartBeat, beatsPerSample);

    if (hasSplit)
    {
        resolveQueuedBoundaryNow (transport, queuedBoundaryBeat_, output, splitSample);

        const int remaining = numSamples - splitSample;
        if (remaining > 0)
            collectActiveClip (output, splitSample, remaining, blockStartBeat + static_cast<double> (splitSample) * beatsPerSample,
                                beatsPerSample);
    }
}

//==============================================================================
// Capture

void Track::feedLiveInput (const juce::MidiBuffer& input, double blockStartBeat, double beatsPerSample)
{
    recordBuffer_.feedBlock (input, blockStartBeat, beatsPerSample);
}

bool Track::captureIntoSlot (int row, const HostTransportState& transport, int numBars)
{
    auto clip = recordBuffer_.captureLastBars (transport.positionQuarterNotes, numBars, transport.barLengthQuarterNotes);
    if (clip == nullptr)
        return false;

    setClip (row, std::move (clip));
    return true;
}

//==============================================================================
// Serialization

juce::ValueTree Track::toValueTree() const
{
    juce::ValueTree tree ("Track");
    tree.setProperty ("name", name_, nullptr);
    tree.setProperty ("colour", static_cast<int> (colourARGB_), nullptr);
    tree.setProperty ("muted", muted_, nullptr);
    tree.setProperty ("armed", armed_, nullptr);
    tree.setProperty ("defaultQuantize", static_cast<int> (defaultQuantize_), nullptr);
    tree.setProperty ("legato", legatoMode_, nullptr);

    for (int i = 0; i < getNumRows(); ++i)
    {
        if (auto* clip = getClip (i))
        {
            auto clipTree = clip->toValueTree();
            clipTree.setProperty ("slot", i, nullptr);
            tree.appendChild (clipTree, nullptr);
        }
    }

    return tree;
}

void Track::fromValueTree (const juce::ValueTree& tree)
{
    name_ = tree.getProperty ("name", juce::String()).toString();
    colourARGB_ = static_cast<juce::uint32> (static_cast<int> (tree.getProperty ("colour", 0)));
    muted_ = static_cast<bool> (tree.getProperty ("muted", false));
    armed_ = static_cast<bool> (tree.getProperty ("armed", false));
    defaultQuantize_ = static_cast<LaunchQuantize> (
        static_cast<int> (tree.getProperty ("defaultQuantize", static_cast<int> (LaunchQuantize::OneBar))));
    legatoMode_ = static_cast<bool> (tree.getProperty ("legato", false));

    cells_.clear();
    activeSlot_ = queuedSlot_ = -1;
    stopQueued_ = hasQueuedBoundary_ = repeating_ = false;
    followTargetBarIndex_ = -1;

    int maxSlot = -1;
    for (const auto& child : tree)
        if (child.hasType ("Clip"))
            maxSlot = juce::jmax (maxSlot, static_cast<int> (child.getProperty ("slot", -1)));

    if (maxSlot >= 0)
        ensureRowCount (maxSlot + 1);

    for (const auto& child : tree)
    {
        if (! child.hasType ("Clip"))
            continue;

        const int slot = static_cast<int> (child.getProperty ("slot", -1));
        if (slot < 0)
            continue;

        if (auto clip = ClipFactory::fromValueTree (child))
            setClip (slot, std::move (clip));
    }
}

} // namespace reaperlooper
