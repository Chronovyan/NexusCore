#pragma once

#include "interfaces/ICollaborativeEditing.hpp"
#include "interfaces/IWebSocketClient.hpp"
#include "interfaces/IWebSocketCallback.hpp"
#include "interfaces/ICRDT.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <chrono>
#include <functional>

namespace ai_editor {

/**
 * @class CollaborativeClient
 * @brief Implementation of the ICollaborativeEditing interface for real-time collaboration.
 * 
 * This class manages the collaborative editing functionality, including:
 * - Connecting to the collaboration server
 * - Managing the CRDT for conflict-free editing
 * - Sending and receiving operations, cursor positions, and selections
 * - Handling presence of remote users
 */
class CollaborativeClient : public ICollaborativeEditing, public IWebSocketCallback {
public:
    /**
     * @brief Constructor
     * @param webSocketClient The WebSocket client to use for communication
     * @param crdt The CRDT to use for conflict-free editing
     */
    CollaborativeClient(
        std::shared_ptr<IWebSocketClient> webSocketClient,
        std::shared_ptr<ICRDT> crdt = nullptr);
    
    /**
     * @brief Destructor
     */
    virtual ~CollaborativeClient();
    
    // ICollaborativeEditing interface implementation
    bool connect(const std::string& serverUrl, const std::string& sessionId, const std::string& userId) override;
    bool disconnect() override;
    bool isConnected() const override;
    
    void registerDocumentChangeCallback(DocumentChangeCallback callback) override;
    void registerCursorChangeCallback(CursorChangeCallback callback) override;
    void registerSelectionChangeCallback(SelectionChangeCallback callback) override;
    void registerPresenceChangeCallback(PresenceChangeCallback callback) override;
    
    bool sendLocalChange(const std::string& change) override;
    bool sendCursorPosition(int line, int column) override;
    bool sendSelection(int startLine, int startColumn, int endLine, int endColumn) override;
    
    std::string getSessionId() const override;
    std::string getUserId() const override;
    std::vector<RemoteUser> getConnectedUsers() const override;
    
    // IWebSocketCallback interface implementation
    void onConnect(const std::string& connectionId) override;
    void onDisconnect(const std::string& connectionId, int code, const std::string& reason) override;
    void onMessage(const WebSocketMessage& message) override;
    void onError(const std::string& connectionId, const std::string& error) override;

private:
    // Helper methods
    bool sendAuthMessage();
    bool sendSyncRequest();
    void handleOperationMessage(const WebSocketMessage& message);
    void handleCursorMessage(const WebSocketMessage& message);
    void handleSelectionMessage(const WebSocketMessage& message);
    void handlePresenceMessage(const WebSocketMessage& message);
    void handleSyncMessage(const WebSocketMessage& message);
    void startHeartbeat();
    void stopHeartbeat();
    
    // Member variables
    std::shared_ptr<IWebSocketClient> webSocketClient_;
    std::shared_ptr<ICRDT> crdt_;
    
    std::string sessionId_;
    std::string documentId_;
    std::string userId_;
    
    // Callbacks
    DocumentChangeCallback documentChangeCallback_;
    CursorChangeCallback cursorChangeCallback_;
    SelectionChangeCallback selectionChangeCallback_;
    PresenceChangeCallback presenceChangeCallback_;
    
    // Connected users and their cursor/selection positions
    std::unordered_map<std::string, RemoteUser> connectedUsers_;
    mutable std::mutex usersMutex_;
    
    // Heartbeat
    std::atomic<bool> heartbeatRunning_;
    std::chrono::milliseconds heartbeatInterval_;
    std::thread heartbeatThread_;
    
    // For detecting reconnection
    bool wasConnected_;
};

} // namespace ai_editor 