#pragma once

#include "interfaces/ICollaborationSession.hpp"
#include "interfaces/ICollaborativeEditing.hpp"
#include "interfaces/ITextEditor.hpp"
#include "interfaces/ICRDT.hpp"
#include "interfaces/IUIManager.hpp"

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace ai_editor {

/**
 * @class CollaborationSession
 * @brief Implementation of the ICollaborationSession interface.
 * 
 * This class manages a collaborative editing session, coordinating between:
 * - The text editor (receiving local changes and displaying remote changes)
 * - The collaborative editing client (sending local changes and receiving remote changes)
 * - The CRDT for conflict-free editing
 * - The UI manager for displaying collaborative UI elements (cursors, selections, etc.)
 */
class CollaborationSession : public ICollaborationSession {
public:
    /**
     * @brief Constructor
     * @param textEditor The text editor to use
     * @param collaborativeClient The collaborative editing client to use
     * @param crdt The CRDT to use for conflict-free editing
     * @param uiManager The UI manager to use for displaying collaborative UI elements
     */
    CollaborationSession(
        std::shared_ptr<ITextEditor> textEditor,
        std::shared_ptr<ICollaborativeEditing> collaborativeClient,
        std::shared_ptr<ICRDT> crdt,
        std::shared_ptr<IUIManager> uiManager = nullptr);
    
    /**
     * @brief Destructor
     */
    virtual ~CollaborationSession();
    
    // ICollaborationSession interface implementation
    bool startSession(const std::string& serverUrl, const std::string& sessionId, const std::string& userId) override;
    bool joinSession(const std::string& serverUrl, const std::string& sessionId, const std::string& userId) override;
    bool leaveSession() override;
    bool isInSession() const override;
    
    std::string getSessionId() const override;
    std::string getUserId() const override;
    std::vector<RemoteUser> getConnectedUsers() const override;
    
    void showRemoteCursors(bool show) override;
    void showRemoteSelections(bool show) override;
    
    bool inviteUser(const std::string& userId) override;

private:
    // Helper methods
    void setupCallbacks();
    void cleanupCallbacks();
    
    void handleLocalTextChange(const TextChange& change);
    void handleLocalCursorChange(int line, int column);
    void handleLocalSelectionChange(int startLine, int startColumn, int endLine, int endColumn);
    
    void handleRemoteDocumentChange(const std::string& userId, const std::string& change);
    void handleRemoteCursorChange(const std::string& userId, int line, int column);
    void handleRemoteSelectionChange(const std::string& userId, int startLine, int startColumn, int endLine, int endColumn);
    void handleRemotePresenceChange(const std::vector<RemoteUser>& users);
    
    void updateRemoteCursorsUI();
    void updateRemoteSelectionsUI();
    
    // Member variables
    std::shared_ptr<ITextEditor> textEditor_;
    std::shared_ptr<ICollaborativeEditing> collaborativeClient_;
    std::shared_ptr<ICRDT> crdt_;
    std::shared_ptr<IUIManager> uiManager_;
    
    bool inSession_;
    bool showRemoteCursors_;
    bool showRemoteSelections_;
    
    // Store remote users' information
    std::vector<RemoteUser> remoteUsers_;
    mutable std::mutex usersMutex_;
    
    // Store connection IDs for cleanup
    std::vector<int> connectionIds_;
};

} // namespace ai_editor 