#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

#include "collaboration/WebSocketClient.hpp"
#include "collaboration/CollaborativeClient.hpp"
#include "collaboration/CollaborationSession.hpp"
#include "crdt/CRDT.hpp"
#include "interfaces/ICollaborativeEditing.hpp"
#include "interfaces/IWebSocketCommunication.hpp"
#include "interfaces/IWebSocketClient.hpp"
#include "interfaces/IWebSocketCallback.hpp"
#include "interfaces/ICRDT.hpp"

using namespace ai_editor;
using namespace testing;

// Mock classes
class MockWebSocketClient : public IWebSocketClient {
public:
    MOCK_METHOD(bool, connect, (const std::string&, const std::unordered_map<std::string, std::string>&), (override));
    MOCK_METHOD(bool, disconnect, (int, const std::string&), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));
    MOCK_METHOD(bool, send, (const WebSocketMessage&), (override));
    MOCK_METHOD(bool, sendRaw, (const std::string&), (override));
    MOCK_METHOD(void, setCallback, (std::shared_ptr<IWebSocketCallback>), (override));
    MOCK_METHOD(std::string, getConnectionId, (), (const, override));
    MOCK_METHOD(std::string, getServerUrl, (), (const, override));
};

class MockTextEditor : public ITextEditor {
public:
    MOCK_METHOD(std::string, getContent, (), (const, override));
    MOCK_METHOD(void, setContent, (const std::string&), (override));
    MOCK_METHOD(void, applyChange, (const TextChange&), (override));
    MOCK_METHOD(int, registerTextChangeCallback, (const TextChangeCallback&), (override));
    MOCK_METHOD(int, registerCursorChangeCallback, (const CursorChangeCallback&), (override));
    MOCK_METHOD(int, registerSelectionChangeCallback, (const SelectionChangeCallback&), (override));
    MOCK_METHOD(void, unregisterCallback, (int), (override));
};

class MockUIManager : public IUIManager {
public:
    MOCK_METHOD(void, updateRemoteCursors, (const std::vector<RemoteCursor>&), (override));
    MOCK_METHOD(void, updateRemoteSelections, (const std::vector<RemoteSelection>&), (override));
};

// Test fixture
class CollaborativeEditingTest : public Test {
protected:
    void SetUp() override {
        mockWebSocketClient = std::make_shared<MockWebSocketClient>();
        mockTextEditor = std::make_shared<MockTextEditor>();
        mockUIManager = std::make_shared<MockUIManager>();
        crdt = std::make_shared<CRDT>();
        
        // Setup expectations for MockWebSocketClient
        EXPECT_CALL(*mockWebSocketClient, setCallback(_)).Times(AtLeast(1));
        EXPECT_CALL(*mockWebSocketClient, getConnectionId()).WillRepeatedly(Return("test-connection"));
        EXPECT_CALL(*mockWebSocketClient, getServerUrl()).WillRepeatedly(Return("ws://test-server"));
        
        // Setup expectations for MockTextEditor
        EXPECT_CALL(*mockTextEditor, getContent()).WillRepeatedly(Return("test content"));
        EXPECT_CALL(*mockTextEditor, registerTextChangeCallback(_)).WillRepeatedly(Return(1));
        EXPECT_CALL(*mockTextEditor, registerCursorChangeCallback(_)).WillRepeatedly(Return(2));
        EXPECT_CALL(*mockTextEditor, registerSelectionChangeCallback(_)).WillRepeatedly(Return(3));
        
        collaborativeClient = std::make_shared<CollaborativeClient>(mockWebSocketClient, crdt);
        collaborationSession = std::make_shared<CollaborationSession>(
            mockTextEditor, collaborativeClient, crdt, mockUIManager);
    }
    
    void TearDown() override {
        collaborationSession.reset();
        collaborativeClient.reset();
        crdt.reset();
        mockUIManager.reset();
        mockTextEditor.reset();
        mockWebSocketClient.reset();
    }
    
    std::shared_ptr<MockWebSocketClient> mockWebSocketClient;
    std::shared_ptr<MockTextEditor> mockTextEditor;
    std::shared_ptr<MockUIManager> mockUIManager;
    std::shared_ptr<CRDT> crdt;
    std::shared_ptr<CollaborativeClient> collaborativeClient;
    std::shared_ptr<CollaborationSession> collaborationSession;
};

// Test cases
TEST_F(CollaborativeEditingTest, SessionStartAndStop) {
    // Setup expectations
    EXPECT_CALL(*mockWebSocketClient, connect(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockWebSocketClient, isConnected()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockWebSocketClient, disconnect(_, _)).WillOnce(Return(true));
    
    // Start session
    EXPECT_TRUE(collaborationSession->startSession("ws://test-server", "test-session", "test-user"));
    EXPECT_TRUE(collaborationSession->isInSession());
    EXPECT_EQ(collaborationSession->getSessionId(), "test-session");
    EXPECT_EQ(collaborationSession->getUserId(), "test-user");
    
    // Leave session
    EXPECT_TRUE(collaborationSession->leaveSession());
    EXPECT_FALSE(collaborationSession->isInSession());
}

TEST_F(CollaborativeEditingTest, RemoteCursorAndSelectionDisplay) {
    // Setup expectations
    EXPECT_CALL(*mockWebSocketClient, connect(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockWebSocketClient, isConnected()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockUIManager, updateRemoteCursors(_)).Times(AtLeast(1));
    EXPECT_CALL(*mockUIManager, updateRemoteSelections(_)).Times(AtLeast(1));
    
    // Start session
    EXPECT_TRUE(collaborationSession->startSession("ws://test-server", "test-session", "test-user"));
    
    // Simulate a remote cursor update
    WebSocketMessage cursorMessage;
    cursorMessage.type = WebSocketMessageType::CURSOR;
    cursorMessage.sessionId = "test-session";
    cursorMessage.documentId = "test-session";
    cursorMessage.userId = "remote-user";
    cursorMessage.data["line"] = "10";
    cursorMessage.data["column"] = "20";
    
    collaborativeClient->onMessage(cursorMessage);
    
    // Simulate a remote selection update
    WebSocketMessage selectionMessage;
    selectionMessage.type = WebSocketMessageType::SELECTION;
    selectionMessage.sessionId = "test-session";
    selectionMessage.documentId = "test-session";
    selectionMessage.userId = "remote-user";
    selectionMessage.data["startLine"] = "10";
    selectionMessage.data["startColumn"] = "20";
    selectionMessage.data["endLine"] = "15";
    selectionMessage.data["endColumn"] = "30";
    
    collaborativeClient->onMessage(selectionMessage);
    
    // Toggle cursor and selection visibility
    collaborationSession->showRemoteCursors(false);
    collaborationSession->showRemoteSelections(false);
    
    // Cleanup
    EXPECT_CALL(*mockWebSocketClient, disconnect(_, _)).WillOnce(Return(true));
    EXPECT_TRUE(collaborationSession->leaveSession());
}

TEST_F(CollaborativeEditingTest, LocalTextChangePropagation) {
    // Setup expectations
    EXPECT_CALL(*mockWebSocketClient, connect(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockWebSocketClient, isConnected()).WillRepeatedly(Return(true));
    EXPECT_CALL(*mockWebSocketClient, send(_)).Times(AtLeast(1));
    
    // Start session
    EXPECT_TRUE(collaborationSession->startSession("ws://test-server", "test-session", "test-user"));
    
    // Capture the text change callback
    TextChangeCallback textChangeCallback;
    EXPECT_CALL(*mockTextEditor, registerTextChangeCallback(_))
        .WillOnce(DoAll(SaveArg<0>(&textChangeCallback), Return(1)));
    
    // Reconstruct the session to capture the callback
    collaborationSession = std::make_shared<CollaborationSession>(
        mockTextEditor, collaborativeClient, crdt, mockUIManager);
    EXPECT_TRUE(collaborationSession->startSession("ws://test-server", "test-session", "test-user"));
    
    // Simulate local text change
    TextChange change;
    change.type = TextChangeType::Insert;
    change.position = 0;
    change.text = "Hello, world!";
    
    // Call the callback directly
    textChangeCallback(change);
    
    // Cleanup
    EXPECT_CALL(*mockWebSocketClient, disconnect(_, _)).WillOnce(Return(true));
    EXPECT_TRUE(collaborationSession->leaveSession());
}

int main(int argc, char** argv) {
    InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 