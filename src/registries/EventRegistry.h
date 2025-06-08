#pragma once

#include "../interfaces/plugins/IEventRegistry.hpp"
#include <string>
#include <memory>
#include <functional>
#include <typeindex>
#include <map>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace ai_editor {

/**
 * @class EventRegistry
 * @brief Implementation of the IEventRegistry interface.
 * 
 * This class manages event subscriptions and dispatches events to registered handlers.
 * It provides a type-safe way for plugins to subscribe to and receive notifications
 * about various editor events.
 */
class EventRegistry : public IEventRegistry {
public:
    /**
     * @brief Constructor.
     */
    EventRegistry();
    
    /**
     * @brief Destructor.
     */
    ~EventRegistry() override = default;
    
    /**
     * @brief Unsubscribe from an event.
     * 
     * @param subscriptionId The identifier returned by subscribe().
     * @return true if the subscription was found and removed, false otherwise.
     */
    bool unsubscribe(const std::string& subscriptionId) override;
    
    /**
     * @brief Publish an event to all subscribers.
     * 
     * @param event The event to publish.
     */
    void publish(const EditorEvent& event);
    
protected:
    /**
     * @brief Implementation of subscribe that handles the type erasure.
     * 
     * @param eventType The type index of the event.
     * @param handler The type-erased handler function.
     * @return A unique identifier for the subscription.
     */
    std::string subscribeImpl(const std::type_index& eventType, 
                           std::function<void(const EditorEvent&)> handler) override;
    
    /**
     * @brief Implementation of hasSubscribers that handles the type erasure.
     * 
     * @param eventType The type index of the event.
     * @return true if there are subscribers for the event type, false otherwise.
     */
    bool hasSubscribersImpl(const std::type_index& eventType) const override;
    
    /**
     * @brief Implementation of getSubscriberCount that handles the type erasure.
     * 
     * @param eventType The type index of the event.
     * @return The number of subscribers for the event type.
     */
    size_t getSubscriberCountImpl(const std::type_index& eventType) const override;
    
private:
    /**
     * @brief Generate a unique subscription ID.
     * 
     * @return A unique subscription ID.
     */
    std::string generateSubscriptionId();
    
    /** @brief Mutex for thread-safe access to subscribers */
    mutable std::mutex mutex_;
    
    /** @brief Map of event types to handlers */
    std::map<std::type_index, std::vector<std::pair<std::string, std::function<void(const EditorEvent&)>>>> subscribers_;
    
    /** @brief Map of subscription IDs to event types */
    std::unordered_map<std::string, std::type_index> subscriptionMap_;
    
    /** @brief Counter for generating subscription IDs */
    uint64_t nextSubscriptionId_ = 0;
};

} // namespace ai_editor 