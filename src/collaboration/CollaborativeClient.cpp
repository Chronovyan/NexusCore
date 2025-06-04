#include "collaboration/CollaborativeClient.hpp"
#include "collaboration/WebSocketClient.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <thread>

namespace ai_editor {

using json = nlohmann::json;

CollaborativeClient::CollaborativeClient(
    std::shared_ptr<IWebSocketClient> webSocketClient,
    std::shared_ptr<ICRDT> crdt)
    : webSocketClient_(webSocketClient),
      crdt_(crdt),
      documentChangeCallback_(nullptr),
      cursorChangeCallback_(nullptr),
      selectionChangeCallback_(nullptr),
      presenceChangeCallback_(nullptr),
      heartbeatRunning_(false),
      heartbeatInterval_(std::chrono::milliseconds(30000)),  // 30 seconds
      wasConnected_(false) {
    
    if (webSocketClient_) {
        webSocketClient_->setCallback(std::shared_ptr<IWebSocketCallback>(this, [](IWebSocketCallback*) {}));
    }
}

CollaborativeClient::~CollaborativeClient() {
    stopHeartbeat();
    disconnect();
}

bool CollaborativeClient::connect(
    const std::string& serverUrl,
    const std::string& sessionId,
    const std::string& userId) {
    
    if (!webSocketClient_) {
        return false;
    }
    
    sessionId_ = sessionId;
    documentId_ = sessionId;  // For simplicity, use sessionId as documentId
    userId_ = userId;
    
    // Connect to the WebSocket server
    std::unordered_map<std::string, std::string> headers;
    headers["User-Agent"] = "AI-Editor/1.0";
    
    if (!webSocketClient_->connect(serverUrl, headers)) {
        return false;
    }
    
    return true;
}

bool CollaborativeClient::disconnect() {
    stopHeartbeat();
    
    if (!webSocketClient_ || !webSocketClient_->isConnected()) {
        return false;
    }
    
    return webSocketClient_->disconnect(1000, "Client disconnected");
}

bool CollaborativeClient::isConnected() const {
    return webSocketClient_ && webSocketClient_->isConnected();
}

void CollaborativeClient::registerDocumentChangeCallback(DocumentChangeCallback callback) {
    documentChangeCallback_ = callback;
}

void CollaborativeClient::registerCursorChangeCallback(CursorChangeCallback callback) {
    cursorChangeCallback_ = callback;
}

void CollaborativeClient::registerSelectionChangeCallback(SelectionChangeCallback callback) {
    selectionChangeCallback_ = callback;
}

void CollaborativeClient::registerPresenceChangeCallback(PresenceChangeCallback callback) {
    presenceChangeCallback_ = callback;
}

bool CollaborativeClient::sendLocalChange(const std::string& change) {
    if (!webSocketClient_ || !webSocketClient_->isConnected()) {
        return false;
    }
    
    try {
        // Parse the change as a JSON object
        json changeJson = json::parse(change);
        
        // Create an operation message
        WebSocketMessage message;
        message.type = WebSocketMessageType::OPERATION;
        message.sessionId = sessionId_;
        message.documentId = documentId_;
        message.userId = userId_;
        message.data["operation"] = change;
        message.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return webSocketClient_->send(message);
    } catch (const std::exception& e) {
        std::cerr << "Failed to send local change: " << e.what() << std::endl;
        return false;
    }
}

bool CollaborativeClient::sendCursorPosition(int line, int column) {
    if (!webSocketClient_ || !webSocketClient_->isConnected()) {
        return false;
    }
    
    try {
        // Create a cursor message
        WebSocketMessage message;
        message.type = WebSocketMessageType::CURSOR;
        message.sessionId = sessionId_;
        message.documentId = documentId_;
        message.userId = userId_;
        message.data["line"] = std::to_string(line);
        message.data["column"] = std::to_string(column);
        message.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return webSocketClient_->send(message);
    } catch (const std::exception& e) {
        std::cerr << "Failed to send cursor position: " << e.what() << std::endl;
        return false;
    }
}

bool CollaborativeClient::sendSelection(int startLine, int startColumn, int endLine, int endColumn) {
    if (!webSocketClient_ || !webSocketClient_->isConnected()) {
        return false;
    }
    
    try {
        // Create a selection message
        WebSocketMessage message;
        message.type = WebSocketMessageType::SELECTION;
        message.sessionId = sessionId_;
        message.documentId = documentId_;
        message.userId = userId_;
        message.data["startLine"] = std::to_string(startLine);
        message.data["startColumn"] = std::to_string(startColumn);
        message.data["endLine"] = std::to_string(endLine);
        message.data["endColumn"] = std::to_string(endColumn);
        message.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return webSocketClient_->send(message);
    } catch (const std::exception& e) {
        std::cerr << "Failed to send selection: " << e.what() << std::endl;
        return false;
    }
}

std::string CollaborativeClient::getSessionId() const {
    return sessionId_;
}

std::string CollaborativeClient::getUserId() const {
    return userId_;
}

std::vector<RemoteUser> CollaborativeClient::getConnectedUsers() const {
    std::vector<RemoteUser> users;
    
    std::lock_guard<std::mutex> lock(usersMutex_);
    
    for (const auto& [userId, user] : connectedUsers_) {
        // Skip the current user
        if (userId != userId_) {
            users.push_back(user);
        }
    }
    
    return users;
}

void CollaborativeClient::onConnect(const std::string& connectionId) {
    // Send auth message
    sendAuthMessage();
    
    // Request sync if reconnecting
    if (wasConnected_) {
        sendSyncRequest();
    }
    
    wasConnected_ = true;
    
    // Start heartbeat
    startHeartbeat();
}

void CollaborativeClient::onDisconnect(const std::string& connectionId, int code, const std::string& reason) {
    stopHeartbeat();
    
    // Clear connected users
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        connectedUsers_.clear();
    }
    
    // Notify callbacks
    if (presenceChangeCallback_) {
        presenceChangeCallback_(getConnectedUsers());
    }
}

void CollaborativeClient::onMessage(const WebSocketMessage& message) {
    try {
        switch (message.type) {
            case WebSocketMessageType::OPERATION:
                handleOperationMessage(message);
                break;
            case WebSocketMessageType::CURSOR:
                handleCursorMessage(message);
                break;
            case WebSocketMessageType::SELECTION:
                handleSelectionMessage(message);
                break;
            case WebSocketMessageType::PRESENCE:
                handlePresenceMessage(message);
                break;
            case WebSocketMessageType::SYNC:
                handleSyncMessage(message);
                break;
            case WebSocketMessageType::PING:
                // Send pong response
                WebSocketMessage pongMessage;
                pongMessage.type = WebSocketMessageType::PONG;
                pongMessage.sessionId = sessionId_;
                pongMessage.documentId = documentId_;
                pongMessage.userId = userId_;
                webSocketClient_->send(pongMessage);
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void CollaborativeClient::onError(const std::string& connectionId, const std::string& error) {
    std::cerr << "WebSocket error: " << error << std::endl;
}

bool CollaborativeClient::sendAuthMessage() {
    if (!webSocketClient_ || !webSocketClient_->isConnected()) {
        return false;
    }
    
    try {
        // Create an auth message
        WebSocketMessage message;
        message.type = WebSocketMessageType::AUTH;
        message.sessionId = sessionId_;
        message.documentId = documentId_;
        message.userId = userId_;
        message.data["username"] = userId_;  // Use userId as username for simplicity
        message.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return webSocketClient_->send(message);
    } catch (const std::exception& e) {
        std::cerr << "Failed to send auth message: " << e.what() << std::endl;
        return false;
    }
}

bool CollaborativeClient::sendSyncRequest() {
    if (!webSocketClient_ || !webSocketClient_->isConnected()) {
        return false;
    }
    
    try {
        // Create a sync request message
        WebSocketMessage message;
        message.type = WebSocketMessageType::SYNC;
        message.sessionId = sessionId_;
        message.documentId = documentId_;
        message.userId = userId_;
        message.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return webSocketClient_->send(message);
    } catch (const std::exception& e) {
        std::cerr << "Failed to send sync request: " << e.what() << std::endl;
        return false;
    }
}

void CollaborativeClient::handleOperationMessage(const WebSocketMessage& message) {
    // Skip messages from self
    if (message.userId == userId_) {
        return;
    }
    
    // Extract operation
    if (message.data.find("operation") == message.data.end()) {
        return;
    }
    
    std::string operation = message.data.at("operation");
    
    // Apply operation to CRDT if available
    if (crdt_) {
        try {
            crdt_->applyRemoteOperation(operation);
        } catch (const std::exception& e) {
            std::cerr << "Failed to apply remote operation: " << e.what() << std::endl;
        }
    }
    
    // Notify callback
    if (documentChangeCallback_) {
        documentChangeCallback_(message.userId, operation);
    }
}

void CollaborativeClient::handleCursorMessage(const WebSocketMessage& message) {
    // Skip messages from self
    if (message.userId == userId_) {
        return;
    }
    
    // Extract cursor position
    if (message.data.find("line") == message.data.end() || 
        message.data.find("column") == message.data.end()) {
        return;
    }
    
    int line = std::stoi(message.data.at("line"));
    int column = std::stoi(message.data.at("column"));
    
    // Update user information
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        if (connectedUsers_.find(message.userId) == connectedUsers_.end()) {
            // New user
            RemoteUser user;
            user.userId = message.userId;
            user.username = message.userId;  // Use userId as username for simplicity
            user.cursorLine = line;
            user.cursorColumn = column;
            connectedUsers_[message.userId] = user;
        } else {
            // Existing user
            connectedUsers_[message.userId].cursorLine = line;
            connectedUsers_[message.userId].cursorColumn = column;
        }
    }
    
    // Notify callbacks
    if (cursorChangeCallback_) {
        cursorChangeCallback_(message.userId, line, column);
    }
}

void CollaborativeClient::handleSelectionMessage(const WebSocketMessage& message) {
    // Skip messages from self
    if (message.userId == userId_) {
        return;
    }
    
    // Extract selection
    if (message.data.find("startLine") == message.data.end() || 
        message.data.find("startColumn") == message.data.end() ||
        message.data.find("endLine") == message.data.end() || 
        message.data.find("endColumn") == message.data.end()) {
        return;
    }
    
    int startLine = std::stoi(message.data.at("startLine"));
    int startColumn = std::stoi(message.data.at("startColumn"));
    int endLine = std::stoi(message.data.at("endLine"));
    int endColumn = std::stoi(message.data.at("endColumn"));
    
    // Update user information
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        if (connectedUsers_.find(message.userId) == connectedUsers_.end()) {
            // New user
            RemoteUser user;
            user.userId = message.userId;
            user.username = message.userId;  // Use userId as username for simplicity
            user.selectionStartLine = startLine;
            user.selectionStartColumn = startColumn;
            user.selectionEndLine = endLine;
            user.selectionEndColumn = endColumn;
            user.hasSelection = true;
            connectedUsers_[message.userId] = user;
        } else {
            // Existing user
            connectedUsers_[message.userId].selectionStartLine = startLine;
            connectedUsers_[message.userId].selectionStartColumn = startColumn;
            connectedUsers_[message.userId].selectionEndLine = endLine;
            connectedUsers_[message.userId].selectionEndColumn = endColumn;
            connectedUsers_[message.userId].hasSelection = true;
        }
    }
    
    // Notify callbacks
    if (selectionChangeCallback_) {
        selectionChangeCallback_(message.userId, startLine, startColumn, endLine, endColumn);
    }
}

void CollaborativeClient::handlePresenceMessage(const WebSocketMessage& message) {
    // Skip messages from self
    if (message.userId == userId_) {
        return;
    }
    
    // Extract presence information
    if (message.data.find("status") == message.data.end()) {
        return;
    }
    
    std::string status = message.data.at("status");
    
    if (status == "joined") {
        // User joined
        std::lock_guard<std::mutex> lock(usersMutex_);
        
        if (connectedUsers_.find(message.userId) == connectedUsers_.end()) {
            // New user
            RemoteUser user;
            user.userId = message.userId;
            user.username = message.userId;  // Use userId as username for simplicity
            connectedUsers_[message.userId] = user;
        }
    } else if (status == "left") {
        // User left
        {
            std::lock_guard<std::mutex> lock(usersMutex_);
            connectedUsers_.erase(message.userId);
        }
    }
    
    // Notify callbacks
    if (presenceChangeCallback_) {
        presenceChangeCallback_(getConnectedUsers());
    }
}

void CollaborativeClient::handleSyncMessage(const WebSocketMessage& message) {
    // Skip messages from self
    if (message.userId == userId_) {
        return;
    }
    
    // Extract document state
    if (message.data.find("state") == message.data.end()) {
        return;
    }
    
    std::string state = message.data.at("state");
    
    // Apply state to CRDT if available
    if (crdt_) {
        try {
            crdt_->fromJson(state);
        } catch (const std::exception& e) {
            std::cerr << "Failed to apply document state: " << e.what() << std::endl;
        }
    }
    
    // Notify callback
    if (documentChangeCallback_) {
        documentChangeCallback_(message.userId, state);
    }
}

void CollaborativeClient::startHeartbeat() {
    if (heartbeatRunning_) {
        return;
    }
    
    heartbeatRunning_ = true;
    
    heartbeatThread_ = std::thread([this]() {
        while (heartbeatRunning_ && webSocketClient_ && webSocketClient_->isConnected()) {
            // Send ping message
            WebSocketMessage pingMessage;
            pingMessage.type = WebSocketMessageType::PING;
            pingMessage.sessionId = sessionId_;
            pingMessage.documentId = documentId_;
            pingMessage.userId = userId_;
            webSocketClient_->send(pingMessage);
            
            // Sleep for heartbeat interval
            std::this_thread::sleep_for(heartbeatInterval_);
        }
    });
}

void CollaborativeClient::stopHeartbeat() {
    heartbeatRunning_ = false;
    
    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
}

} // namespace ai_editor 