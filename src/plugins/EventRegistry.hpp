#pragma once

#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <typeindex>
#include <random>
#include "../interfaces/plugins/IEventRegistry.hpp"
#include "../AppDebugLog.h"

/**
 * @brief Implementation of the IEventRegistry interface.
 * 
 * This class manages event subscriptions and dispatches events to subscribers.
 */
class EventRegistry : public IEventRegistry {
public:
    /**
     * @brief Constructor.
     */
    EventRegistry() {
        LOG_INFO("EventRegistry initialized");
    }

    /**
     * @brief Destructor.
     */
    ~EventRegistry() {
        LOG_INFO("EventRegistry destroyed");
    }

    /**
     * @brief Unsubscribe from an event.
     *
     * @param subscriptionId The identifier returned by subscribe().
     * @return true if the subscription was found and removed, false otherwise.
     */
    bool unsubscribe(const std::string& subscriptionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = subscriptionMap_.find(subscriptionId);
        if (it == subscriptionMap_.end()) {
            LOG_WARNING("Subscription with ID '" + subscriptionId + "' not found");
            return false;
        }
        
        auto& typeIndex = it->second.first;
        auto& handlers = handlerMap_[typeIndex];
        
        // Find and remove the handler
        auto handlerIt = std::find_if(handlers.begin(), handlers.end(),
            [&subscriptionId](const auto& pair) {
                return pair.first == subscriptionId;
            });
        
        if (handlerIt != handlers.end()) {
            handlers.erase(handlerIt);
        }
        
        // Remove the subscription
        subscriptionMap_.erase(it);
        
        LOG_INFO("Unsubscribed from event: " + subscriptionId);
        return true;
    }

    /**
     * @brief Dispatch an event to all subscribers.
     *
     * @tparam EventType The type of event to dispatch.
     * @param event The event to dispatch.
     */
    template<typename EventType>
    void dispatch(const EventType& event) {
        std::vector<std::function<void(const EditorEvent&)>> handlers;
        
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto typeIndex = std::type_index(typeid(EventType));
            auto it = handlerMap_.find(typeIndex);
            if (it == handlerMap_.end()) {
                return;  // No subscribers
            }
            
            // Make a copy of the handlers to avoid holding the lock during dispatch
            for (const auto& pair : it->second) {
                handlers.push_back(pair.second);
            }
        }
        
        // Call all handlers
        for (const auto& handler : handlers) {
            try {
                handler(event);
            } catch (const std::exception& e) {
                LOG_ERROR("Exception in event handler: " + std::string(e.what()));
            }
        }
    }

protected:
    /**
     * @brief Implementation of subscribe that handles the type erasure.
     *
     * @param eventType The type index of the event.
     * @param handler The type-erased handler function.
     * @return A unique identifier for the subscription.
     */
    std::string subscribeImpl(const std::type_index& eventType,
                           std::function<void(const EditorEvent&)> handler) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Generate a unique subscription ID
        std::string subscriptionId = generateSubscriptionId();
        
        // Store the handler
        handlerMap_[eventType].emplace_back(subscriptionId, handler);
        
        // Store the mapping from subscription ID to event type
        subscriptionMap_[subscriptionId] = std::make_pair(eventType, handler);
        
        LOG_INFO("Subscribed to event: " + std::string(eventType.name()) + " with ID: " + subscriptionId);
        return subscriptionId;
    }

    /**
     * @brief Implementation of hasSubscribers that handles the type erasure.
     *
     * @param eventType The type index of the event.
     * @return true if there are subscribers for the event type, false otherwise.
     */
    bool hasSubscribersImpl(const std::type_index& eventType) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = handlerMap_.find(eventType);
        return it != handlerMap_.end() && !it->second.empty();
    }

    /**
     * @brief Implementation of getSubscriberCount that handles the type erasure.
     * 
     * @param eventType The type index of the event.
     * @return The number of subscribers for the event type.
     */
    size_t getSubscriberCountImpl(const std::type_index& eventType) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = handlerMap_.find(eventType);
        if (it == handlerMap_.end()) {
            return 0;
        }
        
        return it->second.size();
    }

private:
    /**
     * @brief Generate a unique subscription ID.
     * 
     * @return A unique subscription ID.
     */
    std::string generateSubscriptionId() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        static const char* hexChars = "0123456789abcdef";
        
        std::string uuid;
        uuid.reserve(36);
        
        for (int i = 0; i < 8; ++i) {
            uuid += hexChars[dis(gen)];
        }
        uuid += '-';
        for (int i = 0; i < 4; ++i) {
            uuid += hexChars[dis(gen)];
        }
        uuid += '-';
        for (int i = 0; i < 4; ++i) {
            uuid += hexChars[dis(gen)];
        }
        uuid += '-';
        for (int i = 0; i < 4; ++i) {
            uuid += hexChars[dis(gen)];
        }
        uuid += '-';
        for (int i = 0; i < 12; ++i) {
            uuid += hexChars[dis(gen)];
        }
        
        return uuid;
    }

    using HandlerPair = std::pair<std::string, std::function<void(const EditorEvent&)>>;
    using TypeIndexPair = std::pair<std::type_index, std::function<void(const EditorEvent&)>>;
    
    std::unordered_map<std::type_index, std::vector<HandlerPair>> handlerMap_;
    std::unordered_map<std::string, TypeIndexPair> subscriptionMap_;
    mutable std::mutex mutex_;  // Protects all data structures
}; 