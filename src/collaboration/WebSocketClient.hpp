#pragma once

#include "interfaces/IWebSocketCommunication.hpp"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <string>

namespace ai_editor {

/**
 * @class WebSocketClient
 * @brief Implementation of IWebSocketClient using Boost.Beast
 * 
 * This class provides a WebSocket client implementation using the Boost.Beast library.
 * It handles connection management, message sending/receiving, and reconnection logic.
 */
class WebSocketClient : public IWebSocketClient {
public:
    /**
     * @brief Constructor
     */
    WebSocketClient();
    
    /**
     * @brief Destructor
     */
    virtual ~WebSocketClient();
    
    // IWebSocketClient implementation
    
    /**
     * @brief Connect to a WebSocket server
     * 
     * @param url The server URL
     * @param headers Additional HTTP headers
     * @return bool True if the connection was initiated successfully
     */
    bool connect(
        const std::string& url,
        const std::unordered_map<std::string, std::string>& headers = {}) override;
    
    /**
     * @brief Disconnect from the server
     * 
     * @param code The close code
     * @param reason The close reason
     * @return bool True if the disconnection was initiated successfully
     */
    bool disconnect(int code = 1000, const std::string& reason = "") override;
    
    /**
     * @brief Check if the client is connected
     * 
     * @return bool True if connected
     */
    bool isConnected() const override;
    
    /**
     * @brief Send a message
     * 
     * @param message The message to send
     * @return bool True if the message was sent successfully
     */
    bool send(const WebSocketMessage& message) override;
    
    /**
     * @brief Send a raw string
     * 
     * @param data The data to send
     * @return bool True if the data was sent successfully
     */
    bool sendRaw(const std::string& data) override;
    
    /**
     * @brief Set the callback
     * 
     * @param callback The callback
     */
    void setCallback(std::shared_ptr<IWebSocketCallback> callback) override;
    
    /**
     * @brief Get the connection ID
     * 
     * @return std::string The connection ID
     */
    std::string getConnectionId() const override;
    
    /**
     * @brief Get the server URL
     * 
     * @return std::string The server URL
     */
    std::string getServerUrl() const override;

private:
    // Internal types
    using tcp = boost::asio::ip::tcp;
    using error_code = boost::beast::error_code;
    using websocket = boost::beast::websocket::stream<tcp::socket>;
    
    // Internal methods
    void runIoContext();
    void onConnect(error_code ec);
    void doRead();
    void onRead(error_code ec, std::size_t bytes_transferred);
    void doWrite();
    void onWrite(error_code ec, std::size_t bytes_transferred);
    void parseUrl(const std::string& url, std::string& host, std::string& port, std::string& path);
    void scheduleReconnect();
    void handleReconnect();
    
    // Member variables
    std::shared_ptr<IWebSocketCallback> callback_;
    std::string connectionId_;
    std::string serverUrl_;
    std::string host_;
    std::string port_;
    std::string path_;
    
    // Boost.Asio and Beast objects
    boost::asio::io_context io_context_;
    std::unique_ptr<websocket> ws_;
    boost::asio::ip::tcp::resolver resolver_;
    
    // Threading and synchronization
    std::thread io_thread_;
    std::mutex write_mutex_;
    std::queue<std::string> write_queue_;
    std::condition_variable write_cv_;
    std::atomic<bool> connected_;
    std::atomic<bool> connecting_;
    std::atomic<bool> stopping_;
    
    // Reconnection
    std::atomic<bool> should_reconnect_;
    unsigned int reconnect_attempts_;
    std::chrono::milliseconds reconnect_delay_;
    
    // Read buffer
    boost::beast::flat_buffer read_buffer_;
};

} // namespace ai_editor 