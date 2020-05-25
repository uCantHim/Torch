#include "QueueProvider.h"

#include <optional>
#include <iostream>

#include "basics/VulkanDebug.h"

using namespace vkb::phys_device_properties;



std::string queueTypeToString(queue_type type)
{
    switch (type)
    {
    case queue_type::graphics:
        return "graphics";
    case queue_type::compute:
        return "compute";
    case queue_type::transfer:
        return "transfer";
    case queue_type::sparseMemory:
        return "sparse memory";
    case queue_type::protectedMemory:
        return "protected memory";
    case queue_type::presentation:
        return "presentation";
    default:
        return "ERROR in queueTypeToString()!";
    }
}


/*
Returns the position of the smallest element in a vector. */
template<typename T>
size_t findMinIndex(const std::vector<T>& vals)
{
    if (vals.empty()) throw std::runtime_error("Vector was empty.");

    T minVal = vals[0];
    size_t minIndex = 0;
    for (size_t i = 1; i < vals.size(); i++) {
        if (vals[i] < minVal) {
            minVal = vals[i];
            minIndex = i;
        }
    }

    return minIndex;
}


/*
Finds the most specialized queue family for a specific queue type.
That means the */
std::optional<familyIndex> findMostSpecialized(queue_type type, const std::vector<QueueFamily>& families)
{
    std::vector<familyIndex> possibleFamilies; // Indices into families
    std::vector<uint32_t> numAdditionalCapabilities; // Corresponds to possibleFamilies

    for (familyIndex i = 0; i < families.size(); i++)
    {
        const auto& family = families[i];
        if (family.isCapable(type)) {
            possibleFamilies.push_back(i);
            numAdditionalCapabilities.push_back(0);
        }
        else continue;

        // Test for other supported types
        for (auto t = static_cast<uint32_t>(queue_type::graphics);
            t < vkb::numQueueTypes_v;
            t++)
        {
            if (t == static_cast<uint32_t>(type)) continue;
            if (family.isCapable(static_cast<queue_type>(t))) numAdditionalCapabilities.back()++;
        }
    }

    if (possibleFamilies.empty()) {
        return std::nullopt;
    }
    familyIndex resultQueue = possibleFamilies[findMinIndex(numAdditionalCapabilities)];
    return resultQueue;
}


// -------------------------------- //
//        Queue provider class        //
// -------------------------------- //

vkb::QueueProvider::QueueProvider(const Device& device)
{
    auto queriedQueues = findQueues(device);

    // Store available queues in map
    for (const auto& [family, queues] : queriedQueues)
    {
        queueFamilies.push_back(family);

        // Build [capability => queue] array (works like a dict in this case)
        for (const auto& queue : queues) {
            if (family.isCapable(queue_type::graphics)) {
                queuesPerCapability[static_cast<size_t>(queue_type::graphics)].push_back(queue);
            }
            if (family.isCapable(queue_type::compute)) {
                queuesPerCapability[static_cast<size_t>(queue_type::compute)].push_back(queue);
            }
            if (family.isCapable(queue_type::transfer)) {
                queuesPerCapability[static_cast<size_t>(queue_type::transfer)].push_back(queue);
            }
            if (family.isCapable(queue_type::sparseMemory)) {
                queuesPerCapability[static_cast<size_t>(queue_type::sparseMemory)].push_back(queue);
            }
            if (family.isCapable(queue_type::protectedMemory)) {
                queuesPerCapability[static_cast<size_t>(queue_type::protectedMemory)].push_back(queue);
            }
            if (family.isCapable(queue_type::presentation)) {
                queuesPerCapability[static_cast<size_t>(queue_type::presentation)].push_back(queue);
            }
        }
    }

    // Look for specialized queues
    std::vector<std::optional<familyIndex>> capabilityFamilyIndices = {
        findMostSpecialized(queue_type::graphics,            queueFamilies),
        findMostSpecialized(queue_type::compute,            queueFamilies),
        findMostSpecialized(queue_type::transfer,            queueFamilies),
        findMostSpecialized(queue_type::sparseMemory,        queueFamilies),
        findMostSpecialized(queue_type::protectedMemory,    queueFamilies),
        findMostSpecialized(queue_type::presentation,        queueFamilies),
    };

    // Use the 0-th queue as a fallback queue for when all other
    // queues are occupied.
    std::vector<uint32_t> usedQueues(queueFamilies.size(), 1);
    int type = -1; // used for logging
    for (auto capabilityIndex : capabilityFamilyIndices)
    {
        type++;
        if (!capabilityIndex.has_value()) continue;

        familyIndex capability = capabilityIndex.value();
        const auto& family = queueFamilies[capability];

        auto nextQueue = usedQueues[family.index];
        usedQueues[family.index]++;
        if (nextQueue >= family.queueCount) {
            nextQueue = 0;
        }

        primaryQueues[type] = queriedQueues[family.index].second[nextQueue];
        primaryQueueFamilies[type] = family.index;

        // log
        if constexpr (enableVerboseLogging)
        {
            std::cout << "Chose queue " << nextQueue << " from family #" << family.index
                << " as the primary " << queueTypeToString(static_cast<queue_type>(type)) << " queue.\n";
        }
    }
}

auto vkb::QueueProvider::getQueue(queue_type type) const noexcept -> const vk::Queue&
{
    return primaryQueues[static_cast<size_t>(type)];
}


auto vkb::QueueProvider::getQueueFamilyIndex(queue_type type) const noexcept -> familyIndex
{
    return primaryQueueFamilies[static_cast<size_t>(type)];
}


size_t vkb::QueueProvider::getQueueFamilyCount() const noexcept
{
    return queueFamilies.size();
}


auto vkb::QueueProvider::getQueueFamilies() const noexcept -> const std::vector<phys_device_properties::QueueFamily>&
{
    return queueFamilies;
}


auto vkb::QueueProvider::findQueues(const Device& device) const
    -> std::vector<std::pair<phys_device_properties::QueueFamily, std::vector<vk::Queue>>>
{
    const auto uniqueQueueFamilies = device.getPhysicalDevice().getUniqueQueueFamilies();

    std::vector<std::pair<phys_device_properties::QueueFamily, std::vector<vk::Queue>>>
    createdQueues(uniqueQueueFamilies.size());
    for (const auto& family : uniqueQueueFamilies)
    {
        createdQueues[family.index].first = family;
        for (uint32_t queueIndex = 0; queueIndex < family.queueCount; queueIndex++)
        {
            auto newQueue = device->getQueue(family.index, queueIndex);
            createdQueues[family.index].second.push_back(newQueue);
        }
    }

    return createdQueues;
}
