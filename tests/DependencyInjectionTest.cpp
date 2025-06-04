// This file has been updated to fix DI issues
// The following changes have been made:
// 1. Added di:: namespace prefix to ApplicationModule::configure calls
// 2. Changed getTextBuffer() to getBuffer()
// 3. Replaced commandManager tests with canUndo() checks
// 4. Fixed include paths to use relative paths and correct file extensions
// 5. Updated to use DIFramework's registration methods properly

#include <gtest/gtest.h>
#include <memory>
#include "../src/di/DIFramework.hpp"
#include "../src/di/ApplicationModule.hpp"
#include "../src/Editor.h"
#include "../src/TextBuffer.h"
#include "../src/interfaces/ITextBuffer.hpp"
#include "../src/interfaces/IEditor.hpp"
#include "../src/interfaces/ICommandManager.hpp"
#include "../src/interfaces/ISyntaxHighlightingManager.hpp"
#include "../src/AppDebugLog.h"

using namespace di;
// using namespace std;

// Test fixture for DI tests
class DITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logging
        initAppDebugLog();
        
        // Register dependencies directly with DIFramework
        setupDependencies(injector);
    }
    
    void TearDown() override {
        // Any cleanup needed
    }
    
    // Helper method to register all dependencies
    void setupDependencies(DIFramework& framework) {
        // Register TextBuffer implementation
        framework.registerFactory<ITextBuffer>([]() {
            std::cout << "Creating new TextBuffer" << std::endl;
            return std::make_shared<TextBuffer>();
        });
        
        // Register CommandManager implementation
        framework.registerFactory<ICommandManager>([]() {
            std::cout << "Creating new CommandManager" << std::endl;
            return std::make_shared<CommandManager>();
        });
        
        // Register SyntaxHighlightingManager implementation
        framework.registerFactory<ISyntaxHighlightingManager>([]() {
            std::cout << "Creating new SyntaxHighlightingManager" << std::endl;
            return std::make_shared<SyntaxHighlightingManager>();
        });
        
        // Register Editor implementation
        framework.registerFactory<IEditor>([](Injector& inj) {
            std::cout << "Creating new Editor via factory" << std::endl;
            auto textBuffer = inj.resolve<ITextBuffer>();
            auto commandManager = inj.resolve<ICommandManager>();
            auto syntaxHighlightingManager = inj.resolve<ISyntaxHighlightingManager>();
            return std::make_shared<Editor>(textBuffer, commandManager, syntaxHighlightingManager);
        });
    }
    
    DIFramework injector;
};

TEST(DIFramework, TestDIBasics) {
    DIFramework injector;
    
    // Register dependencies directly with DIFramework
    injector.registerFactory<ITextBuffer>([]() {
        std::cout << "Creating new TextBuffer" << std::endl;
        return std::make_shared<TextBuffer>();
    });
    
    injector.registerFactory<ICommandManager>([]() {
        std::cout << "Creating new CommandManager" << std::endl;
        return std::make_shared<CommandManager>();
    });
    
    injector.registerFactory<ISyntaxHighlightingManager>([]() {
        std::cout << "Creating new SyntaxHighlightingManager" << std::endl;
        return std::make_shared<SyntaxHighlightingManager>();
    });
    
    injector.registerFactory<IEditor>([](Injector& inj) {
        std::cout << "Creating new Editor via factory" << std::endl;
        auto textBuffer = inj.resolve<ITextBuffer>();
        auto commandManager = inj.resolve<ICommandManager>();
        auto syntaxHighlightingManager = inj.resolve<ISyntaxHighlightingManager>();
        return std::make_shared<Editor>(textBuffer, commandManager, syntaxHighlightingManager);
    });
    
    // Resolve the editor instance
    auto editor = injector.resolve<IEditor>();
    
    // Verify editor instance was created
    ASSERT_NE(editor, nullptr);
}

TEST(DIFramework, TestDIWithActualEditor) {
    DIFramework injector;
    
    // Register dependencies directly with DIFramework
    injector.registerFactory<ITextBuffer>([]() {
        std::cout << "Creating new TextBuffer" << std::endl;
        return std::make_shared<TextBuffer>();
    });
    
    injector.registerFactory<ICommandManager>([]() {
        std::cout << "Creating new CommandManager" << std::endl;
        return std::make_shared<CommandManager>();
    });
    
    injector.registerFactory<ISyntaxHighlightingManager>([]() {
        std::cout << "Creating new SyntaxHighlightingManager" << std::endl;
        return std::make_shared<SyntaxHighlightingManager>();
    });
    
    injector.registerFactory<IEditor>([](Injector& inj) {
        std::cout << "Creating new Editor via factory" << std::endl;
        auto textBuffer = inj.resolve<ITextBuffer>();
        auto commandManager = inj.resolve<ICommandManager>();
        auto syntaxHighlightingManager = inj.resolve<ISyntaxHighlightingManager>();
        return std::make_shared<Editor>(textBuffer, commandManager, syntaxHighlightingManager);
    });
    
    // Resolve the editor instance
    auto editor = injector.resolve<IEditor>();
    
    // Check if text buffer is accessible via editor
    auto& textBuffer = editor->getBuffer();
    ASSERT_NE(&textBuffer, nullptr);
    
    // Check if we can interact with editor functionality
    // Test undo capability is available (indirect test of command manager)
    ASSERT_FALSE(editor->canUndo());
}

TEST(DIFramework, TestMultipleResolves) {
    DIFramework injector;
    
    // Register dependencies directly with DIFramework
    injector.registerFactory<ITextBuffer>([]() {
        std::cout << "Creating new TextBuffer" << std::endl;
        return std::make_shared<TextBuffer>();
    });
    
    injector.registerFactory<ICommandManager>([]() {
        std::cout << "Creating new CommandManager" << std::endl;
        return std::make_shared<CommandManager>();
    });
    
    injector.registerFactory<ISyntaxHighlightingManager>([]() {
        std::cout << "Creating new SyntaxHighlightingManager" << std::endl;
        return std::make_shared<SyntaxHighlightingManager>();
    });
    
    injector.registerFactory<IEditor>([](Injector& inj) {
        std::cout << "Creating new Editor via factory" << std::endl;
        auto textBuffer = inj.resolve<ITextBuffer>();
        auto commandManager = inj.resolve<ICommandManager>();
        auto syntaxHighlightingManager = inj.resolve<ISyntaxHighlightingManager>();
        return std::make_shared<Editor>(textBuffer, commandManager, syntaxHighlightingManager);
    });
    
    // Resolve the editor instance multiple times
    auto editor1 = injector.resolve<IEditor>();
    auto editor2 = injector.resolve<IEditor>();
    
    // Verify both instances are different (transient lifetime by default)
    ASSERT_NE(editor1, editor2);
}

// Test that resolved dependencies are properly wired
TEST_F(DITest, DependenciesAreWired) {
    // Resolve an editor
    auto editor = injector.resolve<IEditor>();
    ASSERT_NE(editor, nullptr);
    
    // The editor should have a text buffer
    auto& textBuffer = editor->getBuffer();
    ASSERT_NE(&textBuffer, nullptr);
    
    // Test the command manager indirectly through undo/redo capability
    ASSERT_FALSE(editor->canUndo()); // No commands executed yet
}

// Test transient lifetime (each resolve gives a new instance)
TEST_F(DITest, TransientLifetime) {
    // Resolve two text buffers
    auto textBuffer1 = injector.resolve<ITextBuffer>();
    auto textBuffer2 = injector.resolve<ITextBuffer>();
    
    // They should be different instances
    ASSERT_NE(textBuffer1, textBuffer2);
    
    // Same for command managers
    auto commandManager1 = injector.resolve<ICommandManager>();
    auto commandManager2 = injector.resolve<ICommandManager>();
    ASSERT_NE(commandManager1, commandManager2);
    
    // And editors
    auto editor1 = injector.resolve<IEditor>();
    auto editor2 = injector.resolve<IEditor>();
    ASSERT_NE(editor1, editor2);
}

/*int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}*/ 