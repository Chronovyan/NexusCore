#ifndef IEVENT_REGISTRY_HPP
#define IEVENT_REGISTRY_HPP

#include <string>
#include <memory>
#include <functional>
#include <typeindex>
#include <map>
#include <vector>
#include "../ITextBuffer.hpp"

// Forward declarations
class Editor;

/**
 * @brief Base class for all editor events.
 */
struct EditorEvent {
    virtual ~EditorEvent() = default;
};

/**
 * @brief Event fired when a document is opened.
 */
struct DocumentOpenedEvent : public EditorEvent {
    std::string filePath;                  ///< Path to the opened file
    std::shared_ptr<ITextBuffer> buffer;   ///< The text buffer containing the document content
    
    DocumentOpenedEvent(const std::string& path, std::shared_ptr<ITextBuffer> buf)
        : filePath(path), buffer(buf) {}
};

/**
 * @brief Event fired when a document is saved.
 */
struct DocumentSavedEvent : public EditorEvent {
    std::string filePath;                  ///< Path to the saved file
    std::shared_ptr<ITextBuffer> buffer;   ///< The text buffer containing the document content
    
    DocumentSavedEvent(const std::string& path, std::shared_ptr<ITextBuffer> buf)
        : filePath(path), buffer(buf) {}
};

/**
 * @brief Event fired when a document is closed.
 */
struct DocumentClosedEvent : public EditorEvent {
    std::string filePath;  ///< Path to the closed file
    
    explicit DocumentClosedEvent(const std::string& path)
        : filePath(path) {}
};

/**
 * @brief Event fired when the cursor position changes.
 */
struct CursorMovedEvent : public EditorEvent {
    size_t line;           ///< New cursor line
    size_t column;         ///< New cursor column
    std::string word;      ///< Word under the cursor (if any)
    Editor* editor;        ///< The editor instance
    
    CursorMovedEvent(size_t l, size_t c, const std::string& w, Editor* ed)
        : line(l), column(c), word(w), editor(ed) {}
};

/**
 * @brief Event fired when text is selected.
 */
struct TextSelectedEvent : public EditorEvent {
    size_t startLine;      ///< Selection start line
    size_t startColumn;    ///< Selection start column
    size_t endLine;        ///< Selection end line
    size_t endColumn;      ///< Selection end column
    std::string text;      ///< Selected text
    Editor* editor;        ///< The editor instance
    
    TextSelectedEvent(size_t sl, size_t sc, size_t el, size_t ec, 
                     const std::string& t, Editor* ed)
        : startLine(sl), startColumn(sc), endLine(el), endColumn(ec),
          text(t), editor(ed) {}
};

/**
 * @brief Event fired when text is modified.
 */
struct TextModifiedEvent : public EditorEvent {
    size_t line;               ///< Line where modification occurred
    size_t column;             ///< Column where modification occurred
    std::string oldText;       ///< Text before modification
    std::string newText;       ///< Text after modification
    Editor* editor;            ///< The editor instance
    
    TextModifiedEvent(size_t l, size_t c, const std::string& ot,
                     const std::string& nt, Editor* ed)
        : line(l), column(c), oldText(ot), newText(nt), editor(ed) {}
};

/**
 * @brief Event fired when a plugin is loaded.
 */
struct PluginLoadedEvent : public EditorEvent {
    std::string pluginId;      ///< ID of the loaded plugin
    std::string pluginName;    ///< Name of the loaded plugin
    
    PluginLoadedEvent(const std::string& id, const std::string& name)
        : pluginId(id), pluginName(name) {}
};

/**
 * @brief Event fired when a plugin is unloaded.
 */
struct PluginUnloadedEvent : public EditorEvent {
    std::string pluginId;      ///< ID of the unloaded plugin
    std::string pluginName;    ///< Name of the unloaded plugin
    
    PluginUnloadedEvent(const std::string& id, const std::string& name)
        : pluginId(id), pluginName(name) {}
};

/**
 * @brief Type alias for event handler functions.
 */
template<typename EventType>
using EventHandler = std::function<void(const EventType&)>;

/**
 * @brief Interface for registering and managing event handlers.
 * 
 * Plugins can use this interface to subscribe to editor events.
 */
class IEventRegistry {
public:
    /**
     * @brief Virtual destructor to ensure proper cleanup of derived classes.
     */
    virtual ~IEventRegistry() = default;
    
    /**
     * @brief Subscribe to an event type with a handler function.
     * 
     * @tparam EventType The type of event to subscribe to.
     * @param handler The function to call when the event occurs.
     * @return A unique identifier for the subscription that can be used to unsubscribe.
     */
    template<typename EventType>
    std::string subscribe(const EventHandler<EventType>& handler) {
        return subscribeImpl(std::type_index(typeid(EventType)), 
                          [handler](const EditorEvent& event) {
                              handler(static_cast<const EventType&>(event));
                          });
    }
    
    /**
     * @brief Unsubscribe from an event.
     * 
     * @param subscriptionId The identifier returned by subscribe().
     * @return true if the subscription was found and removed, false otherwise.
     */
    virtual bool unsubscribe(const std::string& subscriptionId) = 0;
    
    /**
     * @brief Check if there are any subscribers for an event type.
     * 
     * @tparam EventType The type of event to check.
     * @return true if there are subscribers for the event type, false otherwise.
     */
    template<typename EventType>
    bool hasSubscribers() const {
        return hasSubscribersImpl(std::type_index(typeid(EventType)));
    }
    
    /**
     * @brief Get the number of subscribers for an event type.
     * 
     * @tparam EventType The type of event to check.
     * @return The number of subscribers for the event type.
     */
    template<typename EventType>
    size_t getSubscriberCount() const {
        return getSubscriberCountImpl(std::type_index(typeid(EventType)));
    }

protected:
    /**
     * @brief Implementation of subscribe that handles the type erasure.
     * 
     * @param eventType The type index of the event.
     * @param handler The type-erased handler function.
     * @return A unique identifier for the subscription.
     */
    virtual std::string subscribeImpl(const std::type_index& eventType, 
                                   std::function<void(const EditorEvent&)> handler) = 0;
    
    /**
     * @brief Implementation of hasSubscribers that handles the type erasure.
     * 
     * @param eventType The type index of the event.
     * @return true if there are subscribers for the event type, false otherwise.
     */
    virtual bool hasSubscribersImpl(const std::type_index& eventType) const = 0;
    
    /**
     * @brief Implementation of getSubscriberCount that handles the type erasure.
     * 
     * @param eventType The type index of the event.
     * @return The number of subscribers for the event type.
     */
    virtual size_t getSubscriberCountImpl(const std::type_index& eventType) const = 0;
};

#endif // IEVENT_REGISTRY_HPP 