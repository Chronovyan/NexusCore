#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <optional>
#include <unordered_map>
#include <chrono>
#include "interfaces/ITextBuffer.hpp"
#include "Position.h"

namespace ai_editor {

// Forward declarations
class ICollaborativeClient;
class ICollaborationServer;
class ICRDTOperation;

/**
 * @enum CollaborationRole
 * @brief Roles a user can have in a collaborative session
 */
enum class CollaborationRole {
    OWNER,        // Document owner, full control
    EDITOR,       // Can edit the document
    VIEWER,       // Read-only access
    ADMIN         // Administrative access
};

/**
 * @enum ConnectionState
 * @brief States of the connection to the collaboration server
 */
enum class ConnectionState {
    DISCONNECTED,    // Not connected
    CONNECTING,      // Attempting to connect
    CONNECTED,       // Connected and authenticated
    RECONNECTING,    // Attempting to reconnect after a disconnect
    ERROR            // Connection error
};

/**
 * @struct CollaboratorInfo
 * @brief Information about a collaborator in a session
 */
struct CollaboratorInfo {
    std::string id;                // Unique identifier for the collaborator
    std::string displayName;       // Display name
    std::string color;             // Color for cursor and selections
    CollaborationRole role;        // Role in the session
    bool active;                   // Whether the collaborator is currently active
    std::chrono::system_clock::time_point lastActive; // When the collaborator was last active
    
    // Optional fields
    std::optional<Position> cursorPosition;  // Current cursor position
    std::optional<std::pair<Position, Position>> selection; // Current selection (start, end)
};

/**
 * @struct ChatMessage
 * @brief A chat message in a collaborative session
 */
struct ChatMessage {
    std::string id;                // Unique message ID
    std::string senderId;          // ID of the sender
    std::string text;              // Message content
    std::chrono::system_clock::time_point timestamp; // When the message was sent
    bool isSystem;                 // Whether it's a system message
};

/**
 * @interface ICRDTDocument
 * @brief Interface for a CRDT-based document
 */
class ICRDTDocument {
public:
    virtual ~ICRDTDocument() = default;
    
    /**
     * @brief Apply a CRDT operation to the document
     * 
     * @param operation The operation to apply
     * @return bool True if the operation was applied successfully
     */
    virtual bool applyOperation(const std::shared_ptr<ICRDTOperation>& operation) = 0;
    
    /**
     * @brief Get the current state of the document as text
     * 
     * @return std::vector<std::string> The document content as lines of text
     */
    virtual std::vector<std::string> getText() const = 0;
    
    /**
     * @brief Get the document's revision vector clock
     * 
     * @return std::unordered_map<std::string, uint64_t> The vector clock (client ID -> clock value)
     */
    virtual std::unordered_map<std::string, uint64_t> getVectorClock() const = 0;
    
    /**
     * @brief Get operations that have not been acknowledged by the server
     * 
     * @return std::vector<std::shared_ptr<ICRDTOperation>> Unacknowledged operations
     */
    virtual std::vector<std::shared_ptr<ICRDTOperation>> getPendingOperations() const = 0;
    
    /**
     * @brief Create a new document from a snapshot and operations
     * 
     * @param snapshot The document snapshot
     * @param operations Operations to apply to the snapshot
     * @return std::shared_ptr<ICRDTDocument> The reconstructed document
     */
    virtual std::shared_ptr<ICRDTDocument> createFromSnapshot(
        const std::string& snapshot,
        const std::vector<std::shared_ptr<ICRDTOperation>>& operations) = 0;
};

/**
 * @interface ICRDTOperation
 * @brief Interface for CRDT operations
 */
class ICRDTOperation {
public:
    virtual ~ICRDTOperation() = default;
    
    /**
     * @brief Get the type of operation
     * 
     * @return std::string The operation type
     */
    virtual std::string getType() const = 0;
    
    /**
     * @brief Get the client ID that generated this operation
     * 
     * @return std::string The client ID
     */
    virtual std::string getClientId() const = 0;
    
    /**
     * @brief Get the logical clock value for this operation
     * 
     * @return uint64_t The logical clock value
     */
    virtual uint64_t getLogicalClock() const = 0;
    
    /**
     * @brief Serialize the operation to JSON
     * 
     * @return std::string JSON string representation
     */
    virtual std::string serialize() const = 0;
    
    /**
     * @brief Deserialize an operation from JSON
     * 
     * @param json JSON string representation
     * @return std::shared_ptr<ICRDTOperation> Deserialized operation
     */
    static std::shared_ptr<ICRDTOperation> deserialize(const std::string& json);
};

/**
 * @interface ICollaborativeBuffer
 * @brief Interface for a collaborative text buffer
 */
class ICollaborativeTextBuffer : public ITextBuffer {
public:
    virtual ~ICollaborativeTextBuffer() = default;
    
    /**
     * @brief Get the underlying CRDT document
     * 
     * @return std::shared_ptr<ICRDTDocument> The CRDT document
     */
    virtual std::shared_ptr<ICRDTDocument> getCRDTDocument() const = 0;
    
    /**
     * @brief Set the collaboration client for this buffer
     * 
     * @param client The collaboration client
     */
    virtual void setCollaborationClient(std::shared_ptr<ICollaborativeClient> client) = 0;
    
    /**
     * @brief Apply a remote operation
     * 
     * @param operation The operation to apply
     * @return bool True if the operation was applied successfully
     */
    virtual bool applyRemoteOperation(const std::shared_ptr<ICRDTOperation>& operation) = 0;
    
    /**
     * @brief Get the document ID
     * 
     * @return std::string The document ID
     */
    virtual std::string getDocumentId() const = 0;
    
    /**
     * @brief Set the document ID
     * 
     * @param documentId The document ID
     */
    virtual void setDocumentId(const std::string& documentId) = 0;
};

/**
 * @interface ICollaborativeClient
 * @brief Interface for the collaborative client
 */
class ICollaborativeClient {
public:
    virtual ~ICollaborativeClient() = default;
    
    /**
     * @brief Connect to a collaboration server
     * 
     * @param serverUrl The server URL
     * @param userId The user ID
     * @param token Authentication token
     * @return bool True if connection was initiated successfully
     */
    virtual bool connect(
        const std::string& serverUrl,
        const std::string& userId,
        const std::string& token) = 0;
    
    /**
     * @brief Disconnect from the server
     * 
     * @param graceful Whether to perform a graceful disconnect
     * @return bool True if disconnected successfully
     */
    virtual bool disconnect(bool graceful = true) = 0;
    
    /**
     * @brief Get the current connection state
     * 
     * @return ConnectionState The connection state
     */
    virtual ConnectionState getConnectionState() const = 0;
    
    /**
     * @brief Join a collaborative session
     * 
     * @param documentId The document ID
     * @param role Requested role
     * @return bool True if the join request was sent successfully
     */
    virtual bool joinSession(
        const std::string& documentId,
        CollaborationRole role = CollaborationRole::EDITOR) = 0;
    
    /**
     * @brief Leave the current session
     * 
     * @return bool True if the leave request was sent successfully
     */
    virtual bool leaveSession() = 0;
    
    /**
     * @brief Send a CRDT operation to the server
     * 
     * @param operation The operation to send
     * @return bool True if the operation was sent successfully
     */
    virtual bool sendOperation(const std::shared_ptr<ICRDTOperation>& operation) = 0;
    
    /**
     * @brief Send a cursor position update
     * 
     * @param position The cursor position
     * @return bool True if the update was sent successfully
     */
    virtual bool sendCursorPosition(const Position& position) = 0;
    
    /**
     * @brief Send a selection update
     * 
     * @param start The selection start position
     * @param end The selection end position
     * @return bool True if the update was sent successfully
     */
    virtual bool sendSelection(const Position& start, const Position& end) = 0;
    
    /**
     * @brief Send a chat message
     * 
     * @param message The message text
     * @return bool True if the message was sent successfully
     */
    virtual bool sendChatMessage(const std::string& message) = 0;
    
    /**
     * @brief Get all collaborators in the current session
     * 
     * @return std::vector<CollaboratorInfo> Information about all collaborators
     */
    virtual std::vector<CollaboratorInfo> getCollaborators() const = 0;
    
    /**
     * @brief Get chat history
     * 
     * @param limit Maximum number of messages to retrieve
     * @param before Only get messages before this ID
     * @return std::vector<ChatMessage> Chat messages
     */
    virtual std::vector<ChatMessage> getChatHistory(
        size_t limit = 50,
        const std::string& before = "") const = 0;
    
    /**
     * @brief Set callback for when a remote operation is received
     * 
     * @param callback The callback function
     */
    virtual void setOperationReceivedCallback(
        std::function<void(const std::shared_ptr<ICRDTOperation>&)> callback) = 0;
    
    /**
     * @brief Set callback for when a collaborator's cursor position changes
     * 
     * @param callback The callback function
     */
    virtual void setCursorUpdateCallback(
        std::function<void(const std::string&, const Position&)> callback) = 0;
    
    /**
     * @brief Set callback for when a collaborator's selection changes
     * 
     * @param callback The callback function
     */
    virtual void setSelectionUpdateCallback(
        std::function<void(const std::string&, const Position&, const Position&)> callback) = 0;
    
    /**
     * @brief Set callback for when a chat message is received
     * 
     * @param callback The callback function
     */
    virtual void setChatMessageCallback(
        std::function<void(const ChatMessage&)> callback) = 0;
    
    /**
     * @brief Set callback for when a collaborator joins or leaves
     * 
     * @param callback The callback function
     */
    virtual void setCollaboratorUpdateCallback(
        std::function<void(const CollaboratorInfo&, bool)> callback) = 0;
    
    /**
     * @brief Set callback for connection state changes
     * 
     * @param callback The callback function
     */
    virtual void setConnectionStateCallback(
        std::function<void(ConnectionState)> callback) = 0;
};

/**
 * @interface ICollaborationService
 * @brief Interface for the collaboration service
 */
class ICollaborationService {
public:
    virtual ~ICollaborationService() = default;
    
    /**
     * @brief Create a collaborative text buffer
     * 
     * @param documentId The document ID
     * @return std::shared_ptr<ICollaborativeTextBuffer> The collaborative text buffer
     */
    virtual std::shared_ptr<ICollaborativeTextBuffer> createCollaborativeBuffer(
        const std::string& documentId) = 0;
    
    /**
     * @brief Get the collaboration client
     * 
     * @return std::shared_ptr<ICollaborativeClient> The collaboration client
     */
    virtual std::shared_ptr<ICollaborativeClient> getClient() const = 0;
    
    /**
     * @brief Start hosting a collaborative session
     * 
     * @param buffer The buffer to share
     * @param documentId The document ID (optional, will be generated if empty)
     * @return std::string The document ID for the session
     */
    virtual std::string hostSession(
        std::shared_ptr<ITextBuffer> buffer,
        const std::string& documentId = "") = 0;
    
    /**
     * @brief Join an existing collaborative session
     * 
     * @param documentId The document ID
     * @return std::shared_ptr<ICollaborativeTextBuffer> The collaborative buffer
     */
    virtual std::shared_ptr<ICollaborativeTextBuffer> joinSession(
        const std::string& documentId) = 0;
    
    /**
     * @brief End the current session
     * 
     * @return bool True if the session was ended successfully
     */
    virtual bool endSession() = 0;
    
    /**
     * @brief Get the current session document ID
     * 
     * @return std::optional<std::string> The document ID, if in a session
     */
    virtual std::optional<std::string> getCurrentSessionId() const = 0;
    
    /**
     * @brief Get information about the current session
     * 
     * @return std::unordered_map<std::string, std::string> Session information
     */
    virtual std::unordered_map<std::string, std::string> getSessionInfo() const = 0;
    
    /**
     * @brief Set server URL
     * 
     * @param url The server URL
     */
    virtual void setServerUrl(const std::string& url) = 0;
    
    /**
     * @brief Get server URL
     * 
     * @return std::string The server URL
     */
    virtual std::string getServerUrl() const = 0;
};

} // namespace ai_editor 