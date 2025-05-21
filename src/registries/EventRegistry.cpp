#include "EventRegistry.h"
#include "../AppDebugLog.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace ai_editor {

EventRegistry::EventRegistry()
    : subscribers_(),
      subscriptionMap_(),
      nextSubscriptionId_(0) {
    LOG_DEBUG("EventRegistry initialized");
}

std::string EventRegistry::subscribeImpl(const std::type_index& eventType,
                                      std::function<void(const EditorEvent&)> handler) {
    if (!handler) {
        LOG_ERROR("EventRegistry::subscribeImpl called with null handler");
        return "";
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Generate a unique subscription ID
    std::string subscriptionId = generateSubscriptionId();
    
    // Add the handler to the subscribers list for this event type
    subscribers_[eventType].emplace_back(subscriptionId, std::move(handler));
    
    // Map the subscription ID to the event type for fast lookup during unsubscribe
    subscriptionMap_[subscriptionId] = eventType;
    
    LOG_DEBUG("EventRegistry: New subscription " + subscriptionId + " for event type " + eventType.name());
    
    return subscriptionId;
}

bool EventRegistry::unsubscribe(const std::string& subscriptionId) {
    if (subscriptionId.empty()) {
        LOG_ERROR("EventRegistry::unsubscribe called with empty subscription ID");
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find the event type for this subscription ID
    auto it = subscriptionMap_.find(subscriptionId);
    if (it == subscriptionMap_.end()) {
        LOG_WARNING("EventRegistry::unsubscribe: Unknown subscription ID: " + subscriptionId);
        return false;
    }
    
    const std::type_index& eventType = it->second;
    
    // Find and remove the subscriber from the list
    auto& subscribers = subscribers_[eventType];
    auto subscriberIt = std::find_if(subscribers.begin(), subscribers.end(),
                                    [&subscriptionId](const auto& pair) {
                                        return pair.first == subscriptionId;
                                    });
    
    if (subscriberIt == subscribers.end()) {
        LOG_ERROR("EventRegistry::unsubscribe: Inconsistent state - subscription ID in map but not in subscribers list");
        return false;
    }
    
    // Remove the subscriber
    subscribers.erase(subscriberIt);
    
    // If the subscribers list is now empty, remove the event type entry
    if (subscribers.empty()) {
        subscribers_.erase(eventType);
    }
    
    // Remove the subscription ID from the map
    subscriptionMap_.erase(it);
    
    LOG_DEBUG("EventRegistry: Unsubscribed " + subscriptionId + " from event type " + eventType.name());
    
    return true;
}

bool EventRegistry::hasSubscribersImpl(const std::type_index& eventType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = subscribers_.find(eventType);
    return it != subscribers_.end() && !it->second.empty();
}

size_t EventRegistry::getSubscriberCountImpl(const std::type_index& eventType) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = subscribers_.find(eventType);
    if (it == subscribers_.end()) {
        return 0;
    }
    
    return it->second.size();
}

void EventRegistry::publish(const EditorEvent& event) {
    // Use std::type_index to get the type of the event
    std::type_index eventType = std::type_index(typeid(event));
    
    // Collect the handlers that need to be called
    std::vector<std::function<void(const EditorEvent&)>> handlers;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = subscribers_.find(eventType);
        if (it == subscribers_.end() || it->second.empty()) {
            // No subscribers for this event type
            LOG_DEBUG("EventRegistry: No subscribers for event type " + std::string(eventType.name()));
            return;
        }
        
        // Make a copy of the handlers to avoid holding the lock while calling them
        for (const auto& subscriber : it->second) {
            handlers.push_back(subscriber.second);
        }
    }
    
    // Call all the handlers outside the lock
    LOG_DEBUG("EventRegistry: Publishing event of type " + std::string(eventType.name()) + 
              " to " + std::to_string(handlers.size()) + " subscribers");
    
    for (const auto& handler : handlers) {
        try {
            handler(event);
        } catch (const std::exception& e) {
            LOG_ERROR("EventRegistry: Exception in event handler: " + std::string(e.what()));
        } catch (...) {
            LOG_ERROR("EventRegistry: Unknown exception in event handler");
        }
    }
}

std::string EventRegistry::generateSubscriptionId() {
    // Generate a unique subscription ID based on the next ID counter
    uint64_t id = nextSubscriptionId_++;
    
    std::stringstream ss;
    ss << "subscription_" << std::hex << std::setw(16) << std::setfill('0') << id;
    return ss.str();
}

} // namespace ai_editor 