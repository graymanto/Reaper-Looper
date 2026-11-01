#include "ProjectSerializer.h"

namespace reaperlooper::ProjectSerializer
{

namespace
{
    constexpr int kFormatVersion = 1;
}

juce::ValueTree sessionToValueTree (const Session& session)
{
    return session.toValueTree();
}

juce::String sessionToXml (const Session& session)
{
    return sessionToValueTree (session).toXmlString();
}

bool validateValueTree (const juce::ValueTree& tree, juce::String* errorMessage)
{
    if (! tree.isValid() || tree.getType() != juce::Identifier ("Session"))
    {
        if (errorMessage != nullptr)
            *errorMessage = "Not a ReaperLooper session (missing or wrong root element).";
        return false;
    }

    const int version = static_cast<int> (tree.getProperty ("version", 0));
    if (version != kFormatVersion)
    {
        if (errorMessage != nullptr)
            *errorMessage = "Unsupported project version (" + juce::String (version) + ", expected "
                          + juce::String (kFormatVersion) + ").";
        return false;
    }

    return true;
}

bool sessionFromValueTree (Session& session, const juce::ValueTree& tree, juce::String* errorMessage)
{
    if (! validateValueTree (tree, errorMessage))
        return false;

    session.fromValueTree (tree);
    return true;
}

bool sessionFromXml (Session& session, const juce::String& xml, juce::String* errorMessage)
{
    auto tree = juce::ValueTree::fromXml (xml);
    if (! tree.isValid())
    {
        if (errorMessage != nullptr)
            *errorMessage = "Could not parse project XML.";
        return false;
    }

    return sessionFromValueTree (session, tree, errorMessage);
}

} // namespace reaperlooper::ProjectSerializer
