#include "QueueProvider.h"

#include <optional>
#include <iostream>

#include "basics/VulkanDebug.h"

using namespace vkb::phys_device_properties;



std::string queueTypeToString(QueueType type)
{
    switch (type)
    {
    case QueueType::graphics:
        return "graphics";
    case QueueType::compute:
        return "compute";
    case QueueType::transfer:
        return "transfer";
    case QueueType::sparseMemory:
        return "sparse memory";
    case QueueType::protectedMemory:
        return "protected memory";
    case QueueType::presentation:
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
std::optional<QueueFamilyIndex> findMostSpecialized(QueueType type, const std::vector<QueueFamily>& families)
{
    std::vector<QueueFamilyIndex> possibleFamilies; // Indices into families
    std::vector<uint32_t> numAdditionalCapabilities; // Corresponds to possibleFamilies

    for (QueueFamilyIndex i = 0; i < families.size(); i++)
    {
        const auto& family = families[i];
        if (family.isCapable(type)) {
            possibleFamilies.push_back(i);
            numAdditionalCapabilities.push_back(0);
        }
        else continue;

        // Test for other supported types
        for (auto t = static_cast<uint32_t>(QueueType::graphics);
            t < vkb::numQueueTypes_v;
            t++)
        {
            if (t == static_cast<uint32_t>(type)) continue;
            if (family.isCapable(static_cast<QueueType>(t))) numAdditionalCapabilities.back()++;
        }
    }

    if (possibleFamilies.empty()) {
        return std::nullopt;
    }
    QueueFamilyIndex resultQueue = possibleFamilies[findMinIndex(numAdditionalCapabilities)];
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
            if (family.isCapable(QueueType::graphics)) {
                queuesPerCapability[static_cast<size_t>(QueueType::graphics)].push_back(queue);
            }
            if (family.isCapable(QueueType::compute)) {
                queuesPerCapability[static_cast<size_t>(QueueType::compute)].push_back(queue);
            }
            if (family.isCapable(QueueType::transfer)) {
                queuesPerCapability[static_cast<size_t>(QueueType::transfer)].push_back(queue);
            }
            if (family.isCapable(QueueType::sparseMemory)) {
                queuesPerCapability[static_cast<size_t>(QueueType::sparseMemory)].push_back(queue);
            }
            if (family.isCapable(QueueType::protectedMemory)) {
                queuesPerCapability[static_cast<size_t>(QueueType::protectedMemory)].push_back(queue);
            }
            if (family.isCapable(QueueType::presentation)) {
                queuesPerCapability[static_cast<size_t>(QueueType::presentation)].push_back(queue);
            }
        }
    }

    // Look for specialized queues
    std::vector<std::optional<QueueFamilyIndex>> capabilityFamilyIndices = {
        findMostSpecialized(QueueType::graphics,        queueFamilies),
        findMostSpecialized(QueueType::compute,         queueFamilies),
        findMostSpecialized(QueueType::transfer,        queueFamilies),
        findMostSpecialized(QueueType::sparseMemory,    queueFamilies),
        findMostSpecialized(QueueType::protectedMemory, queueFamilies),
        findMostSpecialized(QueueType::presentation,    queueFamilies),
    };

    // Use the 0-th queue as a fallback queue for when all other
    // queues are occupied.
    std::vector<uint32_t> usedQueues(queueFamilies.size(), 1);
    int type = -1;
    for (auto capabilityIndex : capabilityFamilyIndices)
    {
        type++;
        if (!capabilityIndex.has_value()) continue;

        QueueFamilyIndex capability = capabilityIndex.value();
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
                << " as the primary " << queueTypeToString(static_cast<QueueType>(type)) << " queue.\n";
        }
    }
}

auto vkb::QueueProvider::getQueue(QueueType type) const noexcept -> vk::Queue
{
    return primaryQueues[static_cast<size_t>(type)];
}

auto vkb::QueueProvider::getQueueFamilyIndex(QueueType type) const noexcept -> QueueFamilyIndex
{
    return primaryQueueFamilies[static_cast<size_t>(type)];
}

auto vkb::QueueProvider::findQueues(const Device& device) const
    -> std::vector<std::pair<phys_device_properties::QueueFamily, std::vector<vk::Queue>>>
{
    const auto& queueFamilies = device.getPhysicalDevice().queueFamilies;

    std::vector<std::pair<phys_device_properties::QueueFamily, std::vector<vk::Queue>>>
    createdQueues(queueFamilies.size());
    for (const auto& family : queueFamilies)
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
