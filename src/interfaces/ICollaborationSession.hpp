#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <optional>
#include <chrono>
#include "interfaces/ICollaborativeEditing.hpp"
#include "interfaces/IWebSocketCommunication.hpp"
#include "interfaces/ICRDT.hpp"

namespace ai_editor {

/**
 * @enum SessionPermission
 * @brief Permissions that can be granted to users in a session
 */
enum class SessionPermission {
    READ,          // Read the document
    WRITE,         // Write to the document
    CHAT,          // Send chat messages
    INVITE,        // Invite other users
    MANAGE_USERS,  // Manage user permissions
    OWNER          // Full control (owner of the session)
};

/**
 * @struct SessionMetadata
 * @brief Metadata about a collaborative session
 */
struct SessionMetadata {
    std::string id;                     // Session ID
    std::string name;                   // Session name
    std::string documentId;             // Document ID
    std::string ownerId;                // User ID of the owner
    std::chrono::system_clock::time_point createdAt; // When the session was created
    std::chrono::system_clock::time_point updatedAt; // When the session was last updated
    bool isPublic;                      // Whether the session is public
    std::unordered_map<std::string, std::string> metadata; // Additional metadata
    
    /**
     * @brief Create from JSON
     * 
     * @param json JSON string
     * @return SessionMetadata The parsed metadata
     */
    static SessionMetadata fromJson(const std::string& json);
    
    /**
     * @brief Serialize to JSON
     * 
     * @return std::string JSON representation
     */
    std::string toJson() const;
};

/**
 * @interface ICollaborationSessionManager
 * @brief Interface for managing collaborative sessions
 */
class ICollaborationSessionManager {
public:
    virtual ~ICollaborationSessionManager() = default;
    
    /**
     * @brief Create a new session
     * 
     * @param documentId Document ID
     * @param name Session name
     * @param ownerId User ID of the owner
     * @param isPublic Whether the session is public
     * @param metadata Additional metadata
     * @return std::string The session ID
     */
    virtual std::string createSession(
        const std::string& documentId,
        const std::string& name,
        const std::string& ownerId,
        bool isPublic = false,
        const std::unordered_map<std::string, std::string>& metadata = {}) = 0;
    
    /**
     * @brief End a session
     * 
     * @param sessionId Session ID
     * @param userId User ID requesting the end
     * @return bool True if the session was ended successfully
     */
    virtual bool endSession(const std::string& sessionId, const std::string& userId) = 0;
    
    /**
     * @brief Get session metadata
     * 
     * @param sessionId Session ID
     * @return std::optional<SessionMetadata> The session metadata, if found
     */
    virtual std::optional<SessionMetadata> getSessionMetadata(const std::string& sessionId) const = 0;
    
    /**
     * @brief Update session metadata
     * 
     * @param sessionId Session ID
     * @param metadata New metadata
     * @param userId User ID requesting the update
     * @return bool True if the metadata was updated successfully
     */
    virtual bool updateSessionMetadata(
        const std::string& sessionId,
        const SessionMetadata& metadata,
        const std::string& userId) = 0;
    
    /**
     * @brief Add a user to a session
     * 
     * @param sessionId Session ID
     * @param userId User ID to add
     * @param role User role
     * @param invitedBy User ID that invited this user
     * @return bool True if the user was added successfully
     */
    virtual bool addUserToSession(
        const std::string& sessionId,
        const std::string& userId,
        CollaborationRole role,
        const std::string& invitedBy) = 0;
    
    /**
     * @brief Remove a user from a session
     * 
     * @param sessionId Session ID
     * @param userId User ID to remove
     * @param removedBy User ID requesting the removal
     * @return bool True if the user was removed successfully
     */
    virtual bool removeUserFromSession(
        const std::string& sessionId,
        const std::string& userId,
        const std::string& removedBy) = 0;
    
    /**
     * @brief Update a user's role in a session
     * 
     * @param sessionId Session ID
     * @param userId User ID to update
     * @param role New role
     * @param updatedBy User ID requesting the update
     * @return bool True if the role was updated successfully
     */
    virtual bool updateUserRole(
        const std::string& sessionId,
        const std::string& userId,
        CollaborationRole role,
        const std::string& updatedBy) = 0;
    
    /**
     * @brief Get all users in a session
     * 
     * @param sessionId Session ID
     * @return std::vector<CollaboratorInfo> Information about all users in the session
     */
    virtual std::vector<CollaboratorInfo> getSessionUsers(const std::string& sessionId) const = 0;
    
    /**
     * @brief Check if a user is in a session
     * 
     * @param sessionId Session ID
     * @param userId User ID
     * @return bool True if the user is in the session
     */
    virtual bool isUserInSession(const std::string& sessionId, const std::string& userId) const = 0;
    
    /**
     * @brief Get a user's role in a session
     * 
     * @param sessionId Session ID
     * @param userId User ID
     * @return std::optional<CollaborationRole> The user's role, if in the session
     */
    virtual std::optional<CollaborationRole> getUserRole(
        const std::string& sessionId,
        const std::string& userId) const = 0;
    
    /**
     * @brief Get all active sessions
     * 
     * @param userId Optional user ID to filter sessions
     * @return std::vector<SessionMetadata> Information about all active sessions
     */
    virtual std::vector<SessionMetadata> getActiveSessions(
        const std::string& userId = "") const = 0;
};

/**
 * @interface ICollaborationSession
 * @brief Interface for a collaborative session
 */
class ICollaborationSession {
public:
    virtual ~ICollaborationSession() = default;
    
    /**
     * @brief Get the session ID
     * 
     * @return std::string The session ID
     */
    virtual std::string getSessionId() const = 0;
    
    /**
     * @brief Get the document ID
     * 
     * @return std::string The document ID
     */
    virtual std::string getDocumentId() const = 0;
    
    /**
     * @brief Join the session
     * 
     * @param userId User ID
     * @param displayName Display name
     * @param color Color for cursor and selections
     * @return bool True if joined successfully
     */
    virtual bool join(
        const std::string& userId,
        const std::string& displayName,
        const std::string& color) = 0;
    
    /**
     * @brief Leave the session
     * 
     * @param userId User ID
     * @return bool True if left successfully
     */
    virtual bool leave(const std::string& userId) = 0;
    
    /**
     * @brief Send a chat message
     * 
     * @param userId User ID
     * @param message Message text
     * @return bool True if the message was sent successfully
     */
    virtual bool sendChatMessage(const std::string& userId, const std::string& message) = 0;
    
    /**
     * @brief Send a cursor position update
     * 
     * @param userId User ID
     * @param position Cursor position
     * @return bool True if the update was sent successfully
     */
    virtual bool sendCursorPosition(const std::string& userId, const Position& position) = 0;
    
    /**
     * @brief Send a selection update
     * 
     * @param userId User ID
     * @param start Selection start
     * @param end Selection end
     * @return bool True if the update was sent successfully
     */
    virtual bool sendSelection(
        const std::string& userId,
        const Position& start,
        const Position& end) = 0;
    
    /**
     * @brief Send a CRDT operation
     * 
     * @param userId User ID
     * @param operation The operation
     * @return bool True if the operation was sent successfully
     */
    virtual bool sendOperation(
        const std::string& userId,
        const std::shared_ptr<ICRDTOperation>& operation) = 0;
    
    /**
     * @brief Get all collaborators in the session
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
     * @brief Get session metadata
     * 
     * @return SessionMetadata The session metadata
     */
    virtual SessionMetadata getMetadata() const = 0;
    
    /**
     * @brief Update session metadata
     * 
     * @param metadata New metadata
     * @param userId User ID requesting the update
     * @return bool True if the metadata was updated successfully
     */
    virtual bool updateMetadata(
        const SessionMetadata& metadata,
        const std::string& userId) = 0;
    
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
     * @brief Set callback for when a cursor position is updated
     * 
     * @param callback The callback function
     */
    virtual void setCursorUpdateCallback(
        std::function<void(const std::string&, const Position&)> callback) = 0;
    
    /**
     * @brief Set callback for when a selection is updated
     * 
     * @param callback The callback function
     */
    virtual void setSelectionUpdateCallback(
        std::function<void(const std::string&, const Position&, const Position&)> callback) = 0;
    
    /**
     * @brief Set callback for when an operation is received
     * 
     * @param callback The callback function
     */
    virtual void setOperationCallback(
        std::function<void(const std::string&, const std::shared_ptr<ICRDTOperation>&)> callback) = 0;
};

/**
 * @interface ICollaborationSessionFactory
 * @brief Factory for creating collaboration sessions
 */
class ICollaborationSessionFactory {
public:
    virtual ~ICollaborationSessionFactory() = default;
    
    /**
     * @brief Create a new session
     * 
     * @param documentId Document ID
     * @param name Session name
     * @param ownerId User ID of the owner
     * @param isPublic Whether the session is public
     * @param metadata Additional metadata
     * @return std::shared_ptr<ICollaborationSession> The created session
     */
    virtual std::shared_ptr<ICollaborationSession> createSession(
        const std::string& documentId,
        const std::string& name,
        const std::string& ownerId,
        bool isPublic = false,
        const std::unordered_map<std::string, std::string>& metadata = {}) = 0;
    
    /**
     * @brief Get a session by ID
     * 
     * @param sessionId Session ID
     * @return std::shared_ptr<ICollaborationSession> The session, if found
     */
    virtual std::shared_ptr<ICollaborationSession> getSession(
        const std::string& sessionId) = 0;
};

} // namespace ai_editor 