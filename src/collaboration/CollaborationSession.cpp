#include "collaboration/CollaborationSession.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

namespace ai_editor {

using json = nlohmann::json;

CollaborationSession::CollaborationSession(
    std::shared_ptr<ITextEditor> textEditor,
    std::shared_ptr<ICollaborativeEditing> collaborativeClient,
    std::shared_ptr<ICRDT> crdt,
    std::shared_ptr<IUIManager> uiManager)
    : textEditor_(textEditor),
      collaborativeClient_(collaborativeClient),
      crdt_(crdt),
      uiManager_(uiManager),
      inSession_(false),
      showRemoteCursors_(true),
      showRemoteSelections_(true) {
}

CollaborationSession::~CollaborationSession() {
    leaveSession();
}

bool CollaborationSession::startSession(
    const std::string& serverUrl,
    const std::string& sessionId,
    const std::string& userId) {
    
    // Check if already in a session
    if (inSession_) {
        leaveSession();
    }
    
    // Connect to the collaborative editing server
    if (!collaborativeClient_->connect(serverUrl, sessionId, userId)) {
        return false;
    }
    
    // Initialize CRDT with the current document content if needed
    if (crdt_) {
        std::string initialContent = textEditor_->getContent();
        crdt_->initialize(initialContent);
    }
    
    // Setup callbacks
    setupCallbacks();
    
    inSession_ = true;
    return true;
}

bool CollaborationSession::joinSession(
    const std::string& serverUrl,
    const std::string& sessionId,
    const std::string& userId) {
    
    // Functionally the same as startSession for now
    return startSession(serverUrl, sessionId, userId);
}

bool CollaborationSession::leaveSession() {
    if (!inSession_) {
        return false;
    }
    
    // Cleanup callbacks
    cleanupCallbacks();
    
    // Disconnect from the collaborative editing server
    if (collaborativeClient_) {
        collaborativeClient_->disconnect();
    }
    
    // Clear remote users
    {
        std::lock_guard<std::mutex> lock(usersMutex_);
        remoteUsers_.clear();
    }
    
    // Clear remote cursors and selections from UI
    updateRemoteCursorsUI();
    updateRemoteSelectionsUI();
    
    inSession_ = false;
    return true;
}

bool CollaborationSession::isInSession() const {
    return inSession_ && collaborativeClient_ && collaborativeClient_->isConnected();
}

std::string CollaborationSession::getSessionId() const {
    return collaborativeClient_ ? collaborativeClient_->getSessionId() : "";
}

std::string CollaborationSession::getUserId() const {
    return collaborativeClient_ ? collaborativeClient_->getUserId() : "";
}

std::vector<RemoteUser> CollaborationSession::getConnectedUsers() const {
    if (!collaborativeClient_) {
        return {};
    }
    
    return collaborativeClient_->getConnectedUsers();
}

void CollaborationSession::showRemoteCursors(bool show) {
    showRemoteCursors_ = show;
    updateRemoteCursorsUI();
}

void CollaborationSession::showRemoteSelections(bool show) {
    showRemoteSelections_ = show;
    updateRemoteSelectionsUI();
}

bool CollaborationSession::inviteUser(const std::string& userId) {
    // Not implemented yet
    // Would typically involve sending an invitation message
    return false;
}

void CollaborationSession::setupCallbacks() {
    if (!textEditor_ || !collaborativeClient_) {
        return;
    }
    
    // Register text editor callbacks
    int textChangeId = textEditor_->registerTextChangeCallback(
        [this](const TextChange& change) {
            handleLocalTextChange(change);
        });
    connectionIds_.push_back(textChangeId);
    
    int cursorChangeId = textEditor_->registerCursorChangeCallback(
        [this](int line, int column) {
            handleLocalCursorChange(line, column);
        });
    connectionIds_.push_back(cursorChangeId);
    
    int selectionChangeId = textEditor_->registerSelectionChangeCallback(
        [this](int startLine, int startColumn, int endLine, int endColumn) {
            handleLocalSelectionChange(startLine, startColumn, endLine, endColumn);
        });
    connectionIds_.push_back(selectionChangeId);
    
    // Register collaborative client callbacks
    collaborativeClient_->registerDocumentChangeCallback(
        [this](const std::string& userId, const std::string& change) {
            handleRemoteDocumentChange(userId, change);
        });
    
    collaborativeClient_->registerCursorChangeCallback(
        [this](const std::string& userId, int line, int column) {
            handleRemoteCursorChange(userId, line, column);
        });
    
    collaborativeClient_->registerSelectionChangeCallback(
        [this](const std::string& userId, int startLine, int startColumn, int endLine, int endColumn) {
            handleRemoteSelectionChange(userId, startLine, startColumn, endLine, endColumn);
        });
    
    collaborativeClient_->registerPresenceChangeCallback(
        [this](const std::vector<RemoteUser>& users) {
            handleRemotePresenceChange(users);
        });
}

void CollaborationSession::cleanupCallbacks() {
    if (textEditor_) {
        for (int id : connectionIds_) {
            textEditor_->unregisterCallback(id);
        }
    }
    
    connectionIds_.clear();
    
    if (collaborativeClient_) {
        collaborativeClient_->registerDocumentChangeCallback(nullptr);
        collaborativeClient_->registerCursorChangeCallback(nullptr);
        collaborativeClient_->registerSelectionChangeCallback(nullptr);
        collaborativeClient_->registerPresenceChangeCallback(nullptr);
    }
}

void CollaborationSession::handleLocalTextChange(const TextChange& change) {
    if (!inSession_ || !collaborativeClient_ || !crdt_) {
        return;
    }
    
    try {
        // Apply the change to the CRDT
        std::string operation = crdt_->handleLocalOperation(change);
        
        // Send the operation to the server
        collaborativeClient_->sendLocalChange(operation);
    } catch (const std::exception& e) {
        std::cerr << "Error handling local text change: " << e.what() << std::endl;
    }
}

void CollaborationSession::handleLocalCursorChange(int line, int column) {
    if (!inSession_ || !collaborativeClient_) {
        return;
    }
    
    try {
        // Send cursor position to the server
        collaborativeClient_->sendCursorPosition(line, column);
    } catch (const std::exception& e) {
        std::cerr << "Error handling local cursor change: " << e.what() << std::endl;
    }
}

void CollaborationSession::handleLocalSelectionChange(int startLine, int startColumn, int endLine, int endColumn) {
    if (!inSession_ || !collaborativeClient_) {
        return;
    }
    
    try {
        // Send selection to the server
        collaborativeClient_->sendSelection(startLine, startColumn, endLine, endColumn);
    } catch (const std::exception& e) {
        std::cerr << "Error handling local selection change: " << e.what() << std::endl;
    }
}

void CollaborationSession::handleRemoteDocumentChange(const std::string& userId, const std::string& change) {
    if (!inSession_ || !textEditor_ || !crdt_) {
        return;
    }
    
    try {
        // Apply the remote operation to the CRDT
        std::string textChange = crdt_->applyRemoteOperation(change);
        
        // Parse the text change
        json textChangeJson = json::parse(textChange);
        
        // Apply the text change to the editor
        TextChange editorChange;
        editorChange.type = textChangeJson["type"] == "insert" ? TextChangeType::Insert : TextChangeType::Delete;
        editorChange.position = textChangeJson["position"];
        if (editorChange.type == TextChangeType::Insert) {
            editorChange.text = textChangeJson["text"];
        } else {
            editorChange.length = textChangeJson["length"];
        }
        
        textEditor_->applyChange(editorChange);
    } catch (const std::exception& e) {
        std::cerr << "Error handling remote document change: " << e.what() << std::endl;
    }
}

void CollaborationSession::handleRemoteCursorChange(const std::string& userId, int line, int column) {
    if (!inSession_) {
        return;
    }
    
    try {
        // Update remote users
        {
            std::lock_guard<std::mutex> lock(usersMutex_);
            
            // Find the user
            bool found = false;
            for (auto& user : remoteUsers_) {
                if (user.userId == userId) {
                    user.cursorLine = line;
                    user.cursorColumn = column;
                    found = true;
                    break;
                }
            }
            
            // Add the user if not found
            if (!found) {
                RemoteUser user;
                user.userId = userId;
                user.username = userId;  // Use userId as username for simplicity
                user.cursorLine = line;
                user.cursorColumn = column;
                remoteUsers_.push_back(user);
            }
        }
        
        // Update UI
        updateRemoteCursorsUI();
    } catch (const std::exception& e) {
        std::cerr << "Error handling remote cursor change: " << e.what() << std::endl;
    }
}

void CollaborationSession::handleRemoteSelectionChange(
    const std::string& userId,
    int startLine,
    int startColumn,
    int endLine,
    int endColumn) {
    
    if (!inSession_) {
        return;
    }
    
    try {
        // Update remote users
        {
            std::lock_guard<std::mutex> lock(usersMutex_);
            
            // Find the user
            bool found = false;
            for (auto& user : remoteUsers_) {
                if (user.userId == userId) {
                    user.selectionStartLine = startLine;
                    user.selectionStartColumn = startColumn;
                    user.selectionEndLine = endLine;
                    user.selectionEndColumn = endColumn;
                    user.hasSelection = true;
                    found = true;
                    break;
                }
            }
            
            // Add the user if not found
            if (!found) {
                RemoteUser user;
                user.userId = userId;
                user.username = userId;  // Use userId as username for simplicity
                user.selectionStartLine = startLine;
                user.selectionStartColumn = startColumn;
                user.selectionEndLine = endLine;
                user.selectionEndColumn = endColumn;
                user.hasSelection = true;
                remoteUsers_.push_back(user);
            }
        }
        
        // Update UI
        updateRemoteSelectionsUI();
    } catch (const std::exception& e) {
        std::cerr << "Error handling remote selection change: " << e.what() << std::endl;
    }
}

void CollaborationSession::handleRemotePresenceChange(const std::vector<RemoteUser>& users) {
    if (!inSession_) {
        return;
    }
    
    try {
        // Update remote users
        {
            std::lock_guard<std::mutex> lock(usersMutex_);
            remoteUsers_ = users;
        }
        
        // Update UI
        updateRemoteCursorsUI();
        updateRemoteSelectionsUI();
    } catch (const std::exception& e) {
        std::cerr << "Error handling remote presence change: " << e.what() << std::endl;
    }
}

void CollaborationSession::updateRemoteCursorsUI() {
    if (!inSession_ || !uiManager_) {
        return;
    }
    
    try {
        if (showRemoteCursors_) {
            std::vector<RemoteCursor> cursors;
            
            // Create cursor objects
            {
                std::lock_guard<std::mutex> lock(usersMutex_);
                
                for (const auto& user : remoteUsers_) {
                    RemoteCursor cursor;
                    cursor.userId = user.userId;
                    cursor.username = user.username;
                    cursor.line = user.cursorLine;
                    cursor.column = user.cursorColumn;
                    
                    // Generate a color based on userId (simple hash)
                    size_t hash = std::hash<std::string>{}(user.userId);
                    // Generate RGB values between 50 and 200 (not too dark or too light)
                    int r = 50 + (hash % 150);
                    int g = 50 + ((hash >> 8) % 150);
                    int b = 50 + ((hash >> 16) % 150);
                    
                    cursor.color = "#" + 
                        (r < 16 ? "0" : "") + std::to_string(r) +
                        (g < 16 ? "0" : "") + std::to_string(g) +
                        (b < 16 ? "0" : "") + std::to_string(b);
                    
                    cursors.push_back(cursor);
                }
            }
            
            // Update UI
            uiManager_->updateRemoteCursors(cursors);
        } else {
            // Clear cursors
            uiManager_->updateRemoteCursors({});
        }
    } catch (const std::exception& e) {
        std::cerr << "Error updating remote cursors UI: " << e.what() << std::endl;
    }
}

void CollaborationSession::updateRemoteSelectionsUI() {
    if (!inSession_ || !uiManager_) {
        return;
    }
    
    try {
        if (showRemoteSelections_) {
            std::vector<RemoteSelection> selections;
            
            // Create selection objects
            {
                std::lock_guard<std::mutex> lock(usersMutex_);
                
                for (const auto& user : remoteUsers_) {
                    if (user.hasSelection) {
                        RemoteSelection selection;
                        selection.userId = user.userId;
                        selection.username = user.username;
                        selection.startLine = user.selectionStartLine;
                        selection.startColumn = user.selectionStartColumn;
                        selection.endLine = user.selectionEndLine;
                        selection.endColumn = user.selectionEndColumn;
                        
                        // Generate a color based on userId (simple hash)
                        size_t hash = std::hash<std::string>{}(user.userId);
                        // Generate RGB values between 50 and 200 (not too dark or too light)
                        int r = 50 + (hash % 150);
                        int g = 50 + ((hash >> 8) % 150);
                        int b = 50 + ((hash >> 16) % 150);
                        
                        selection.color = "#" + 
                            (r < 16 ? "0" : "") + std::to_string(r) +
                            (g < 16 ? "0" : "") + std::to_string(g) +
                            (b < 16 ? "0" : "") + std::to_string(b);
                        
                        selections.push_back(selection);
                    }
                }
            }
            
            // Update UI
            uiManager_->updateRemoteSelections(selections);
        } else {
            // Clear selections
            uiManager_->updateRemoteSelections({});
        }
    } catch (const std::exception& e) {
        std::cerr << "Error updating remote selections UI: " << e.what() << std::endl;
    }
}

} // namespace ai_editor 