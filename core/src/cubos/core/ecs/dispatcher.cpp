#include <algorithm>

#include <cubos/core/ecs/dispatcher.hpp>

using namespace cubos::core::ecs;

void Dispatcher::SystemSettings::copyFrom(const SystemSettings* other)
{
    std::unique_copy(other->before.tag.begin(), other->before.tag.end(), std::back_inserter(this->before.tag));
    std::unique_copy(other->after.tag.begin(), other->after.tag.end(), std::back_inserter(this->after.tag));
    std::unique_copy(other->before.system.begin(), other->before.system.end(), std::back_inserter(this->before.system));
    std::unique_copy(other->after.system.begin(), other->after.system.end(), std::back_inserter(this->after.system));
    this->conditions |= other->conditions;
}

Dispatcher::~Dispatcher()
{
    for (System* system : mSystems)
    {
        delete system;
    }
}

void Dispatcher::addTag(const std::string& tag)
{
    ENSURE_TAG_SETTINGS(tag);
    mCurrTag = tag;
}

void Dispatcher::tagInheritTag(const std::string& tag)
{
    ENSURE_CURR_TAG();
    ENSURE_TAG_SETTINGS(tag);
    if (std::find(mTagSettings[mCurrTag]->inherits.begin(), mTagSettings[mCurrTag]->inherits.end(), tag) !=
        mTagSettings[mCurrTag]->inherits.end())
    {
        CUBOS_INFO("Tag already inherits from '{}'", tag);
    }
    mTagSettings[mCurrTag]->inherits.push_back(tag);
}

void Dispatcher::tagSetAfterTag(const std::string& tag)
{
    ENSURE_CURR_TAG();
    ENSURE_TAG_SETTINGS(tag);
    // Set curr to run after this tag
    mTagSettings[mCurrTag]->after.tag.push_back(tag);
    // And that tag to run before this tag
    mTagSettings[tag]->before.tag.push_back(mCurrTag);
}

void Dispatcher::tagSetBeforeTag(const std::string& tag)
{
    ENSURE_CURR_TAG();
    ENSURE_TAG_SETTINGS(tag);
    // Set curr to run before this tag
    mTagSettings[mCurrTag]->before.tag.push_back(tag);
    // And that tag to run after this tag
    mTagSettings[tag]->after.tag.push_back(mCurrTag);
}

void Dispatcher::systemAddTag(const std::string& tag)
{
    ENSURE_CURR_SYSTEM();
    ENSURE_TAG_SETTINGS(tag);
    mCurrSystem->tags.insert(tag);
}

void Dispatcher::systemSetAfterTag(const std::string& tag)
{
    ENSURE_CURR_SYSTEM();
    ENSURE_SYSTEM_SETTINGS(mCurrSystem);
    addTag(tag);
    // Set curr to run after this tag
    mCurrSystem->settings->after.tag.push_back(tag);
    // And that tag to run before this system tag
    mTagSettings[tag]->before.system.push_back(mCurrSystem);
}

void Dispatcher::systemSetBeforeTag(const std::string& tag)
{
    ENSURE_CURR_SYSTEM();
    ENSURE_SYSTEM_SETTINGS(mCurrSystem);
    addTag(tag);
    // Set curr to run before this tag
    mCurrSystem->settings->before.tag.push_back(tag);
    // And that tag to run after this system tag
    mTagSettings[tag]->after.system.push_back(mCurrSystem);
}

void Dispatcher::handleTagInheritance(std::shared_ptr<SystemSettings>& settings)
{
    for (auto& parentTag : settings->inherits)
    {
        auto& parentSettings = mTagSettings[parentTag];
        handleTagInheritance(parentSettings);
        settings->copyFrom(parentSettings.get());
    }
    settings->inherits.clear();
}

void Dispatcher::compileChain()
{
    // Implement tag inheritance by copying configs from parent tags
    for (auto& [tag, settings] : mTagSettings)
    {
        handleTagInheritance(settings);
    }

    // Implement system tag settings with custom settings
    for (System* system : mPendingSystems)
    {
        for (const auto& tag : system->tags)
        {
            if (!system->settings)
            {
                system->settings = std::make_shared<SystemSettings>();
            }
            system->settings->copyFrom(mTagSettings[tag].get());
        }
    }

    // Build the system chain
    std::vector<DFSNode> nodes;
    for (System* system : mPendingSystems)
    {
        nodes.push_back(DFSNode{DFSNode::WHITE, system, "", system->settings});
    }

    for (auto& [tag, settings] : mTagSettings)
    {
        nodes.push_back(DFSNode{DFSNode::WHITE, nullptr, tag, settings});
    }

    // Keep running while there are unvisited nodes
    std::vector<DFSNode>::iterator it;
    while ((it = std::find_if(nodes.begin(), nodes.end(),
                              [](const DFSNode& node) { return node.m == DFSNode::WHITE; })) != nodes.end())
    {
        if (dfsVisit(*it, nodes))
        {
            CUBOS_ERROR("Cycle detected in system chain! Ensure there are no circular dependencies!");
            return;
        }
    }

    // The algorithm expects nodes to be added to the head of a list, instead of the back. To save
    // on move operations, just reverse the final list for the same effect.
    std::reverse(mSystems.begin(), mSystems.end());

    CUBOS_INFO("Call chain completed successfully!");
    mPendingSystems.clear();
    mCurrSystem = nullptr;
    mTagSettings.clear();
}

bool Dispatcher::dfsVisit(DFSNode& node, std::vector<DFSNode>& nodes)
{
    switch (node.m)
    {
    case DFSNode::BLACK: // Node has been fully visited already. Nothing else to do.
        return false;
    case DFSNode::GRAY: // Node is being processed. This means there's a cycle.
        return true;
    case DFSNode::WHITE: // Node is unexplored.
    {
        node.m = DFSNode::GRAY;
        if (node.settings)
        {
            // Visit tags first
            for (const std::string& tag : node.settings->before.tag)
            {
                for (auto it = nodes.begin(); it != nodes.end(); it++)
                {
                    if (it->s != nullptr)
                    {
                        if (std::find(it->s->tags.begin(), it->s->tags.end(), tag) != it->s->tags.end())
                        {
                            if (dfsVisit(*it, nodes))
                            {
                                return true;
                            }
                        }
                    }
                    else
                    {
                        if (tag == it->t || std::find(it->settings->inherits.begin(), it->settings->inherits.end(),
                                                      tag) != it->settings->inherits.end())
                        {
                            if (dfsVisit(*it, nodes))
                            {
                                return true;
                            }
                        }
                    }
                }
            }

            // Now visit systems
            for (System* system : node.settings->before.system)
            {
                SystemSettings* settings = system->settings.get();
                for (auto it = nodes.begin(); it != nodes.end(); it++)
                {
                    if (it->settings.get() == settings)
                    {
                        if (dfsVisit(*it, nodes))
                        {
                            return true;
                        }
                    }
                }
            }
        }

        // All children nodes were visited; mark this node as complete
        node.m = DFSNode::BLACK;
        if (node.s != nullptr)
        {
            mSystems.push_back(node.s);
        }
        return false;
    }
    }
    return false;
}

void Dispatcher::callSystems(World& world, CommandBuffer& cmds)
{
    // If the systems haven't been prepared yet, do so now.
    // We can't multi-thread this as the systems require exclusive access to the world to prepare.
    if (!mPrepared)
    {
        for (auto& system : mSystems)
        {
            system->system->prepare(world);
        }

        for (auto& condition : mConditions)
        {
            condition->prepare(world);
        }

        mPrepared = true;
    }

    // Clear conditions bitmasks
    mRunConditions.reset();
    mRetConditions.reset();

    for (auto& system : mSystems)
    {
        // Query for conditions
        bool canRun = true;

        if (system->settings != nullptr)
        {
            auto conditionsMask = system->settings->conditions;
            std::size_t i = 0;
            while (conditionsMask.any())
            {
                if (conditionsMask.test(0))
                {
                    // We have a condition, check if it has run already
                    if (!mRunConditions.test(i))
                    {
                        mRunConditions.set(i);
                        if (mConditions[i]->call(world, cmds))
                        {
                            mRetConditions.set(i);
                        }
                    }
                    // Check if the condition returned true
                    if (!mRetConditions.test(i))
                    {
                        canRun = false;
                        break;
                    }
                }

                i += 1;
                conditionsMask >>= 1;
            }
        }

        if (canRun)
        {
            system->system->call(world, cmds);
        }

        // TODO: Check synchronization concerns when this gets multithreaded
        cmds.commit();
    }
}
