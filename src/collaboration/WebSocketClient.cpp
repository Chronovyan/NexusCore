#include "collaboration/WebSocketClient.hpp"
#include <nlohmann/json.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <regex>
#include <iostream>

namespace ai_editor {

using json = nlohmann::json;

WebSocketClient::WebSocketClient()
    : callback_(nullptr),
      connectionId_(boost::uuids::to_string(boost::uuids::random_generator()())),
      resolver_(io_context_),
      connected_(false),
      connecting_(false),
      stopping_(false),
      should_reconnect_(false),
      reconnect_attempts_(0),
      reconnect_delay_(std::chrono::milliseconds(1000)) {
}

WebSocketClient::~WebSocketClient() {
    stopping_ = true;
    disconnect();
    
    // Wait for IO thread to exit
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}

bool WebSocketClient::connect(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& headers) {
    
    if (connected_ || connecting_) {
        return false;
    }
    
    serverUrl_ = url;
    connecting_ = true;
    
    // Parse URL
    parseUrl(url, host_, port_, path_);
    
    // Start IO thread if not already running
    if (!io_thread_.joinable()) {
        io_thread_ = std::thread(&WebSocketClient::runIoContext, this);
    }
    
    // Resolve the host
    resolver_.async_resolve(
        host_,
        port_,
        [this, headers](boost::beast::error_code ec, tcp::resolver::results_type results) {
            if (ec) {
                connecting_ = false;
                if (callback_) {
                    callback_->onError(connectionId_, "Failed to resolve host: " + ec.message());
                }
                return;
            }
            
            // Create a new WebSocket
            ws_ = std::make_unique<websocket>(io_context_);
            
            // Set timeout options
            ws_->set_option(boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::client));
            
            // Set additional headers
            for (const auto& [key, value] : headers) {
                ws_->set_option(boost::beast::websocket::stream_base::decorator(
                    [key, value](boost::beast::websocket::request_type& req) {
                        req.set(boost::beast::http::field::user_agent, "AI-Editor WebSocketClient");
                        req.set(key, value);
                    }));
            }
            
            // Connect to the TCP endpoint
            boost::asio::async_connect(
                ws_->next_layer(),
                results,
                [this](boost::beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
                    if (ec) {
                        connecting_ = false;
                        ws_.reset();
                        if (callback_) {
                            callback_->onError(connectionId_, "Failed to connect: " + ec.message());
                        }
                        scheduleReconnect();
                        return;
                    }
                    
                    // Perform WebSocket handshake
                    ws_->async_handshake(
                        host_,
                        path_,
                        [this](boost::beast::error_code ec) {
                            onConnect(ec);
                        });
                });
        });
    
    return true;
}

bool WebSocketClient::disconnect(int code, const std::string& reason) {
    if (!connected_ && !connecting_) {
        return false;
    }
    
    should_reconnect_ = false;
    connecting_ = false;
    
    if (ws_) {
        try {
            // Close the WebSocket connection
            boost::beast::error_code ec;
            ws_->close(static_cast<boost::beast::websocket::close_code>(code), ec);
            
            if (ec && callback_) {
                callback_->onError(connectionId_, "Failed to close WebSocket: " + ec.message());
            }
            
            connected_ = false;
            ws_.reset();
            
            if (callback_) {
                callback_->onDisconnect(connectionId_, code, reason);
            }
            
            return true;
        } catch (const std::exception& e) {
            if (callback_) {
                callback_->onError(connectionId_, "Exception during disconnect: " + std::string(e.what()));
            }
            connected_ = false;
            ws_.reset();
            return false;
        }
    }
    
    return true;
}

bool WebSocketClient::isConnected() const {
    return connected_;
}

bool WebSocketClient::send(const WebSocketMessage& message) {
    if (!connected_) {
        return false;
    }
    
    try {
        return sendRaw(message.toJson());
    } catch (const std::exception& e) {
        if (callback_) {
            callback_->onError(connectionId_, "Exception during send: " + std::string(e.what()));
        }
        return false;
    }
}

bool WebSocketClient::sendRaw(const std::string& data) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::lock_guard<std::mutex> lock(write_mutex_);
        write_queue_.push(data);
        write_cv_.notify_one();
        return true;
    } catch (const std::exception& e) {
        if (callback_) {
            callback_->onError(connectionId_, "Exception during sendRaw: " + std::string(e.what()));
        }
        return false;
    }
}

void WebSocketClient::setCallback(std::shared_ptr<IWebSocketCallback> callback) {
    callback_ = callback;
}

std::string WebSocketClient::getConnectionId() const {
    return connectionId_;
}

std::string WebSocketClient::getServerUrl() const {
    return serverUrl_;
}

void WebSocketClient::runIoContext() {
    while (!stopping_) {
        try {
            // Reset any state
            io_context_.restart();
            
            // Run the IO context
            io_context_.run();
            
            // If we get here, the IO context has no more work to do
            if (should_reconnect_ && !stopping_) {
                handleReconnect();
            } else {
                // Sleep a bit before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } catch (const std::exception& e) {
            if (callback_) {
                callback_->onError(connectionId_, "IO thread exception: " + std::string(e.what()));
            }
            
            // Sleep a bit before trying again
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void WebSocketClient::onConnect(boost::beast::error_code ec) {
    connecting_ = false;
    
    if (ec) {
        if (callback_) {
            callback_->onError(connectionId_, "Handshake failed: " + ec.message());
        }
        ws_.reset();
        scheduleReconnect();
        return;
    }
    
    connected_ = true;
    reconnect_attempts_ = 0;
    
    if (callback_) {
        callback_->onConnect(connectionId_);
    }
    
    // Start reading
    doRead();
    
    // Start writing (if there's anything in the queue)
    doWrite();
}

void WebSocketClient::doRead() {
    if (!connected_ || !ws_) {
        return;
    }
    
    ws_->async_read(
        read_buffer_,
        [this](boost::beast::error_code ec, std::size_t bytes_transferred) {
            onRead(ec, bytes_transferred);
        });
}

void WebSocketClient::onRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        connected_ = false;
        
        if (ec == boost::beast::websocket::error::closed) {
            if (callback_) {
                callback_->onDisconnect(connectionId_, 1000, "Connection closed by server");
            }
        } else {
            if (callback_) {
                callback_->onError(connectionId_, "Read error: " + ec.message());
            }
        }
        
        ws_.reset();
        scheduleReconnect();
        return;
    }
    
    // Get the message as a string
    std::string message(
        static_cast<const char*>(read_buffer_.data().data()),
        bytes_transferred);
    
    // Clear the buffer
    read_buffer_.consume(bytes_transferred);
    
    // Parse the message
    try {
        WebSocketMessage parsedMessage = WebSocketMessage::fromJson(message);
        
        if (callback_) {
            callback_->onMessage(parsedMessage);
        }
    } catch (const std::exception& e) {
        if (callback_) {
            callback_->onError(connectionId_, "Failed to parse message: " + std::string(e.what()));
        }
    }
    
    // Continue reading
    doRead();
}

void WebSocketClient::doWrite() {
    if (!connected_ || !ws_) {
        return;
    }
    
    std::unique_lock<std::mutex> lock(write_mutex_);
    
    if (write_queue_.empty()) {
        return;
    }
    
    std::string message = write_queue_.front();
    write_queue_.pop();
    
    lock.unlock();
    
    ws_->async_write(
        boost::asio::buffer(message),
        [this](boost::beast::error_code ec, std::size_t bytes_transferred) {
            onWrite(ec, bytes_transferred);
        });
}

void WebSocketClient::onWrite(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (ec) {
        connected_ = false;
        
        if (callback_) {
            callback_->onError(connectionId_, "Write error: " + ec.message());
        }
        
        ws_.reset();
        scheduleReconnect();
        return;
    }
    
    std::unique_lock<std::mutex> lock(write_mutex_);
    
    if (!write_queue_.empty()) {
        std::string message = write_queue_.front();
        write_queue_.pop();
        
        lock.unlock();
        
        ws_->async_write(
            boost::asio::buffer(message),
            [this](boost::beast::error_code ec, std::size_t bytes_transferred) {
                onWrite(ec, bytes_transferred);
            });
    }
}

void WebSocketClient::parseUrl(const std::string& url, std::string& host, std::string& port, std::string& path) {
    std::regex url_regex("^(wss?)://([^:/]+)(?::([0-9]+))?(/.*)?$");
    std::smatch matches;
    
    if (std::regex_match(url, matches, url_regex)) {
        std::string scheme = matches[1].str();
        host = matches[2].str();
        port = matches[3].str();
        path = matches[4].str();
        
        if (port.empty()) {
            port = (scheme == "wss") ? "443" : "80";
        }
        
        if (path.empty()) {
            path = "/";
        }
    } else {
        // Default values if URL parsing fails
        host = "localhost";
        port = "80";
        path = "/";
    }
}

void WebSocketClient::scheduleReconnect() {
    if (stopping_ || !should_reconnect_) {
        return;
    }
    
    // Calculate delay with exponential backoff
    auto delay = std::chrono::milliseconds(
        std::min<unsigned int>(
            30000,  // Max delay of 30 seconds
            reconnect_delay_.count() * (1 << std::min<unsigned int>(reconnect_attempts_, 5))
        )
    );
    
    // Increment attempt counter
    reconnect_attempts_++;
    
    // Schedule reconnect
    std::thread([this, delay]() {
        std::this_thread::sleep_for(delay);
        
        if (!stopping_ && should_reconnect_) {
            // Try to reconnect
            connect(serverUrl_);
        }
    }).detach();
}

void WebSocketClient::handleReconnect() {
    if (stopping_ || connected_ || connecting_) {
        return;
    }
    
    // Try to reconnect
    should_reconnect_ = false;
    connect(serverUrl_);
}

// Static method to implement WebSocketMessage::fromJson
WebSocketMessage WebSocketMessage::fromJson(const std::string& jsonStr) {
    WebSocketMessage message;
    
    try {
        json j = json::parse(jsonStr);
        
        // Parse message type
        std::string typeStr = j["type"].get<std::string>();
        if (typeStr == "auth") {
            message.type = WebSocketMessageType::AUTH;
        } else if (typeStr == "sync") {
            message.type = WebSocketMessageType::SYNC;
        } else if (typeStr == "operation") {
            message.type = WebSocketMessageType::OPERATION;
        } else if (typeStr == "cursor") {
            message.type = WebSocketMessageType::CURSOR;
        } else if (typeStr == "selection") {
            message.type = WebSocketMessageType::SELECTION;
        } else if (typeStr == "chat") {
            message.type = WebSocketMessageType::CHAT;
        } else if (typeStr == "presence") {
            message.type = WebSocketMessageType::PRESENCE;
        } else if (typeStr == "error") {
            message.type = WebSocketMessageType::ERROR;
        } else if (typeStr == "status") {
            message.type = WebSocketMessageType::STATUS;
        } else if (typeStr == "ping") {
            message.type = WebSocketMessageType::PING;
        } else if (typeStr == "pong") {
            message.type = WebSocketMessageType::PONG;
        } else {
            throw std::runtime_error("Unknown message type: " + typeStr);
        }
        
        // Parse other fields
        if (j.contains("sessionId")) {
            message.sessionId = j["sessionId"].get<std::string>();
        }
        
        if (j.contains("documentId")) {
            message.documentId = j["documentId"].get<std::string>();
        }
        
        if (j.contains("userId")) {
            message.userId = j["userId"].get<std::string>();
        }
        
        if (j.contains("data") && j["data"].is_object()) {
            for (auto it = j["data"].begin(); it != j["data"].end(); ++it) {
                message.data[it.key()] = it.value().get<std::string>();
            }
        }
        
        if (j.contains("timestamp")) {
            message.timestamp = j["timestamp"].get<uint64_t>();
        } else {
            // Use current time if not provided
            message.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
        }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse WebSocketMessage: " + std::string(e.what()));
    }
    
    return message;
}

// Static method to implement WebSocketMessage::toJson
std::string WebSocketMessage::toJson() const {
    json j;
    
    // Convert type to string
    std::string typeStr;
    switch (type) {
        case WebSocketMessageType::AUTH:
            typeStr = "auth";
            break;
        case WebSocketMessageType::SYNC:
            typeStr = "sync";
            break;
        case WebSocketMessageType::OPERATION:
            typeStr = "operation";
            break;
        case WebSocketMessageType::CURSOR:
            typeStr = "cursor";
            break;
        case WebSocketMessageType::SELECTION:
            typeStr = "selection";
            break;
        case WebSocketMessageType::CHAT:
            typeStr = "chat";
            break;
        case WebSocketMessageType::PRESENCE:
            typeStr = "presence";
            break;
        case WebSocketMessageType::ERROR:
            typeStr = "error";
            break;
        case WebSocketMessageType::STATUS:
            typeStr = "status";
            break;
        case WebSocketMessageType::PING:
            typeStr = "ping";
            break;
        case WebSocketMessageType::PONG:
            typeStr = "pong";
            break;
    }
    
    j["type"] = typeStr;
    j["sessionId"] = sessionId;
    j["documentId"] = documentId;
    j["userId"] = userId;
    j["data"] = data;
    j["timestamp"] = timestamp;
    
    return j.dump();
}

} // namespace ai_editor 