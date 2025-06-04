#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <optional>

namespace ai_editor {

/**
 * @enum WebSocketMessageType
 * @brief Types of messages that can be sent over WebSocket
 */
enum class WebSocketMessageType {
    AUTH,         // Authentication
    SYNC,         // Document synchronization
    OPERATION,    // CRDT operation
    CURSOR,       // Cursor position update
    SELECTION,    // Selection update
    CHAT,         // Chat message
    PRESENCE,     // User presence update
    ERROR,        // Error message
    STATUS,       // Status update
    PING,         // Ping message
    PONG          // Pong message
};

/**
 * @struct WebSocketMessage
 * @brief A message sent over WebSocket
 */
struct WebSocketMessage {
    WebSocketMessageType type;                 // Message type
    std::string sessionId;                     // Session ID
    std::string documentId;                    // Document ID
    std::string userId;                        // User ID
    std::unordered_map<std::string, std::string> data; // Message data
    uint64_t timestamp;                        // Message timestamp
    
    /**
     * @brief Create a message from JSON
     * 
     * @param json JSON string
     * @return WebSocketMessage The parsed message
     */
    static WebSocketMessage fromJson(const std::string& json);
    
    /**
     * @brief Serialize to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const;
};

/**
 * @interface IWebSocketCallback
 * @brief Interface for WebSocket callbacks
 */
class IWebSocketCallback {
public:
    virtual ~IWebSocketCallback() = default;
    
    /**
     * @brief Called when a message is received
     * 
     * @param message The received message
     */
    virtual void onMessage(const WebSocketMessage& message) = 0;
    
    /**
     * @brief Called when a connection is established
     * 
     * @param connectionId The connection ID
     */
    virtual void onConnect(const std::string& connectionId) = 0;
    
    /**
     * @brief Called when a connection is closed
     * 
     * @param connectionId The connection ID
     * @param code The close code
     * @param reason The close reason
     */
    virtual void onDisconnect(
        const std::string& connectionId,
        int code,
        const std::string& reason) = 0;
    
    /**
     * @brief Called when an error occurs
     * 
     * @param connectionId The connection ID
     * @param error The error message
     */
    virtual void onError(const std::string& connectionId, const std::string& error) = 0;
};

/**
 * @interface IWebSocketClient
 * @brief Interface for WebSocket client
 */
class IWebSocketClient {
public:
    virtual ~IWebSocketClient() = default;
    
    /**
     * @brief Connect to a WebSocket server
     * 
     * @param url The server URL
     * @param headers Additional HTTP headers
     * @return bool True if the connection was initiated successfully
     */
    virtual bool connect(
        const std::string& url,
        const std::unordered_map<std::string, std::string>& headers = {}) = 0;
    
    /**
     * @brief Disconnect from the server
     * 
     * @param code The close code
     * @param reason The close reason
     * @return bool True if the disconnection was initiated successfully
     */
    virtual bool disconnect(int code = 1000, const std::string& reason = "") = 0;
    
    /**
     * @brief Check if the client is connected
     * 
     * @return bool True if connected
     */
    virtual bool isConnected() const = 0;
    
    /**
     * @brief Send a message
     * 
     * @param message The message to send
     * @return bool True if the message was sent successfully
     */
    virtual bool send(const WebSocketMessage& message) = 0;
    
    /**
     * @brief Send a raw string
     * 
     * @param data The data to send
     * @return bool True if the data was sent successfully
     */
    virtual bool sendRaw(const std::string& data) = 0;
    
    /**
     * @brief Set the callback
     * 
     * @param callback The callback
     */
    virtual void setCallback(std::shared_ptr<IWebSocketCallback> callback) = 0;
    
    /**
     * @brief Get the connection ID
     * 
     * @return std::string The connection ID
     */
    virtual std::string getConnectionId() const = 0;
    
    /**
     * @brief Get the server URL
     * 
     * @return std::string The server URL
     */
    virtual std::string getServerUrl() const = 0;
};

/**
 * @interface IWebSocketServer
 * @brief Interface for WebSocket server
 */
class IWebSocketServer {
public:
    virtual ~IWebSocketServer() = default;
    
    /**
     * @brief Start the server
     * 
     * @param port The port to listen on
     * @param host The host to bind to
     * @return bool True if the server was started successfully
     */
    virtual bool start(uint16_t port, const std::string& host = "0.0.0.0") = 0;
    
    /**
     * @brief Stop the server
     * 
     * @return bool True if the server was stopped successfully
     */
    virtual bool stop() = 0;
    
    /**
     * @brief Check if the server is running
     * 
     * @return bool True if running
     */
    virtual bool isRunning() const = 0;
    
    /**
     * @brief Send a message to a specific client
     * 
     * @param connectionId The connection ID
     * @param message The message to send
     * @return bool True if the message was sent successfully
     */
    virtual bool send(const std::string& connectionId, const WebSocketMessage& message) = 0;
    
    /**
     * @brief Send a raw string to a specific client
     * 
     * @param connectionId The connection ID
     * @param data The data to send
     * @return bool True if the data was sent successfully
     */
    virtual bool sendRaw(const std::string& connectionId, const std::string& data) = 0;
    
    /**
     * @brief Broadcast a message to all clients
     * 
     * @param message The message to broadcast
     * @param excludeConnectionId Optional connection ID to exclude
     * @return bool True if the message was broadcast successfully
     */
    virtual bool broadcast(
        const WebSocketMessage& message,
        const std::string& excludeConnectionId = "") = 0;
    
    /**
     * @brief Broadcast a raw string to all clients
     * 
     * @param data The data to broadcast
     * @param excludeConnectionId Optional connection ID to exclude
     * @return bool True if the data was broadcast successfully
     */
    virtual bool broadcastRaw(
        const std::string& data,
        const std::string& excludeConnectionId = "") = 0;
    
    /**
     * @brief Close a specific client connection
     * 
     * @param connectionId The connection ID
     * @param code The close code
     * @param reason The close reason
     * @return bool True if the connection was closed successfully
     */
    virtual bool closeConnection(
        const std::string& connectionId,
        int code = 1000,
        const std::string& reason = "") = 0;
    
    /**
     * @brief Get all connected clients
     * 
     * @return std::vector<std::string> List of connection IDs
     */
    virtual std::vector<std::string> getConnections() const = 0;
    
    /**
     * @brief Get the number of connected clients
     * 
     * @return size_t The number of connected clients
     */
    virtual size_t getConnectionCount() const = 0;
    
    /**
     * @brief Set the callback
     * 
     * @param callback The callback
     */
    virtual void setCallback(std::shared_ptr<IWebSocketCallback> callback) = 0;
};

/**
 * @interface IWebSocketFactory
 * @brief Factory for creating WebSocket clients and servers
 */
class IWebSocketFactory {
public:
    virtual ~IWebSocketFactory() = default;
    
    /**
     * @brief Create a WebSocket client
     * 
     * @return std::shared_ptr<IWebSocketClient> The created client
     */
    virtual std::shared_ptr<IWebSocketClient> createClient() = 0;
    
    /**
     * @brief Create a WebSocket server
     * 
     * @return std::shared_ptr<IWebSocketServer> The created server
     */
    virtual std::shared_ptr<IWebSocketServer> createServer() = 0;
};

} // namespace ai_editor 