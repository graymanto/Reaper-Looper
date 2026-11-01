#include "Clip.h"

namespace reaperlooper
{

namespace
{
    constexpr int kQuantizeNone     = -1;

    int quantizeToInt (std::optional<LaunchQuantize> q)
    {
        return q.has_value() ? static_cast<int> (*q) : kQuantizeNone;
    }

    std::optional<LaunchQuantize> intToQuantize (int v)
    {
        if (v == kQuantizeNone)
            return std::nullopt;
        return static_cast<LaunchQuantize> (v);
    }
}

juce::ValueTree Clip::toValueTree() const
{
    juce::ValueTree tree ("Clip");
    tree.setProperty ("type", getTypeId(), nullptr);
    tree.setProperty ("name", name_, nullptr);
    tree.setProperty ("looping", looping_, nullptr);
    tree.setProperty ("launchMode", static_cast<int> (launchMode_), nullptr);
    tree.setProperty ("quantizeOverride", quantizeToInt (quantizeOverride_), nullptr);

    tree.setProperty ("followType", static_cast<int> (followAction_.type), nullptr);
    tree.setProperty ("followAfterBars", followAction_.afterBars, nullptr);
    tree.setProperty ("followTargetRow", followAction_.targetRow, nullptr);
    tree.setProperty ("followChance", static_cast<double> (followAction_.chance), nullptr);

    return tree;
}

void Clip::fromValueTree (const juce::ValueTree& tree)
{
    name_ = tree.getProperty ("name", juce::String()).toString();
    looping_ = static_cast<bool> (tree.getProperty ("looping", true));
    launchMode_ = static_cast<LaunchMode> (static_cast<int> (tree.getProperty ("launchMode", static_cast<int> (LaunchMode::Toggle))));
    quantizeOverride_ = intToQuantize (static_cast<int> (tree.getProperty ("quantizeOverride", kQuantizeNone)));

    followAction_.type = static_cast<FollowActionType> (static_cast<int> (tree.getProperty ("followType", static_cast<int> (FollowActionType::None))));
    followAction_.afterBars = static_cast<int> (tree.getProperty ("followAfterBars", 1));
    followAction_.targetRow = static_cast<int> (tree.getProperty ("followTargetRow", -1));
    followAction_.chance = static_cast<float> (static_cast<double> (tree.getProperty ("followChance", 1.0)));
}

} // namespace reaperlooper
