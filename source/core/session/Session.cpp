#include "Session.h"

namespace reaperlooper
{

void Session::prepare (double sampleRate)
{
    sampleRate_ = sampleRate;
    for (auto& t : tracks_)
        t->prepare (sampleRate);
}

int Session::addTrack()
{
    auto track = std::make_unique<Track>();
    track->prepare (sampleRate_);
    track->setDefaultQuantize (globalDefaultQuantize_);
    track->ensureRowCount (getNumRows());
    tracks_.push_back (std::move (track));
    return static_cast<int> (tracks_.size()) - 1;
}

int Session::addScene (const juce::String& name)
{
    sceneNames_.push_back (name);
    const int newRowCount = static_cast<int> (sceneNames_.size());
    for (auto& t : tracks_)
        t->ensureRowCount (newRowCount);
    return newRowCount - 1;
}

juce::String Session::getSceneName (int row) const
{
    return (row >= 0 && row < static_cast<int> (sceneNames_.size())) ? sceneNames_[static_cast<size_t> (row)] : juce::String();
}

void Session::setSceneName (int row, const juce::String& name)
{
    if (row >= 0 && row < static_cast<int> (sceneNames_.size()))
        sceneNames_[static_cast<size_t> (row)] = name;
}

void Session::handleSceneLaunch (int row, const HostTransportState& transport)
{
    for (auto& t : tracks_)
        t->handleSceneLaunchForRow (row, transport);
}

void Session::stopAll (const HostTransportState& transport)
{
    for (auto& t : tracks_)
        t->queueStopAll (transport);
}

void Session::processBlock (std::vector<juce::MidiBuffer>& trackOutputs, int numSamples,
                             const HostTransportState& transport, const juce::MidiBuffer& liveInput)
{
    if (trackOutputs.size() != tracks_.size())
        trackOutputs.resize (tracks_.size());

    const double beatsPerSample = sampleRate_ > 0.0 ? (transport.bpm / 60.0) / sampleRate_ : 0.0;

    for (size_t i = 0; i < tracks_.size(); ++i)
    {
        tracks_[i]->feedLiveInput (liveInput, transport.positionQuarterNotes, beatsPerSample);
        tracks_[i]->processBlock (trackOutputs[i], numSamples, transport);
    }
}

juce::ValueTree Session::toValueTree() const
{
    juce::ValueTree tree ("Session");
    tree.setProperty ("version", 1, nullptr);
    tree.setProperty ("globalDefaultQuantize", static_cast<int> (globalDefaultQuantize_), nullptr);

    for (int row = 0; row < getNumRows(); ++row)
    {
        juce::ValueTree sceneTree ("Scene");
        sceneTree.setProperty ("row", row, nullptr);
        sceneTree.setProperty ("name", sceneNames_[static_cast<size_t> (row)], nullptr);
        tree.appendChild (sceneTree, nullptr);
    }

    for (int i = 0; i < getNumTracks(); ++i)
    {
        auto trackTree = tracks_[static_cast<size_t> (i)]->toValueTree();
        trackTree.setProperty ("index", i, nullptr);
        tree.appendChild (trackTree, nullptr);
    }

    return tree;
}

void Session::fromValueTree (const juce::ValueTree& tree)
{
    tracks_.clear();
    sceneNames_.clear();

    globalDefaultQuantize_ = static_cast<LaunchQuantize> (
        static_cast<int> (tree.getProperty ("globalDefaultQuantize", static_cast<int> (LaunchQuantize::OneBar))));

    int maxRow = -1;
    for (const auto& child : tree)
        if (child.hasType ("Scene"))
            maxRow = juce::jmax (maxRow, static_cast<int> (child.getProperty ("row", -1)));

    sceneNames_.resize (static_cast<size_t> (maxRow + 1));
    for (const auto& child : tree)
    {
        if (! child.hasType ("Scene"))
            continue;
        const int row = static_cast<int> (child.getProperty ("row", -1));
        if (row >= 0)
            sceneNames_[static_cast<size_t> (row)] = child.getProperty ("name", juce::String()).toString();
    }

    int maxTrackIndex = -1;
    for (const auto& child : tree)
        if (child.hasType ("Track"))
            maxTrackIndex = juce::jmax (maxTrackIndex, static_cast<int> (child.getProperty ("index", -1)));

    for (int i = 0; i <= maxTrackIndex; ++i)
        addTrack();

    for (const auto& child : tree)
    {
        if (! child.hasType ("Track"))
            continue;
        const int index = static_cast<int> (child.getProperty ("index", -1));
        if (index >= 0 && index < getNumTracks())
            tracks_[static_cast<size_t> (index)]->fromValueTree (child);
    }

    // Tracks deserialize their own clip slots, which may imply more rows than
    // any Scene entry did (e.g. an older/partial save) - make sure every
    // track ends up with at least getNumRows() rows either way.
    for (auto& t : tracks_)
        t->ensureRowCount (getNumRows());
}

} // namespace reaperlooper
