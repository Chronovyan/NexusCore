#include <gtest/gtest.h>
#include <memory>
#include "di/Injector.hpp"
#include "di/ApplicationModule.hpp"
#include "interfaces/IEditor.hpp"
#include "interfaces/ITextBuffer.hpp"
#include "interfaces/ICommandManager.hpp"
#include "AppDebugLog.h"

// Test fixture for DI tests
class DIFixedTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize logging
        initAppDebugLog();
    }
    
    void TearDown() override {
        // Any cleanup needed
    }
    
    di::Injector injector;
};

// Test basic dependency resolution
TEST_F(DIFixedTest, BasicDependencyResolution) {
    // Configure the injector with our application module
    di::ApplicationModule::configure(injector);
    
    // Test resolving ITextBuffer
    auto textBuffer = injector.resolve<ITextBuffer>();
    ASSERT_NE(textBuffer, nullptr);
    
    // Test resolving ICommandManager
    auto commandManager = injector.resolve<ICommandManager>();
    ASSERT_NE(commandManager, nullptr);
    
    // Test resolving IEditor
    auto editor = injector.resolve<IEditor>();
    ASSERT_NE(editor, nullptr);
}

// Test that resolved dependencies are properly wired
TEST_F(DIFixedTest, DependenciesAreWired) {
    // Configure the injector with our application module
    di::ApplicationModule::configure(injector);
    
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
TEST_F(DIFixedTest, TransientLifetime) {
    // Configure the injector with our application module
    di::ApplicationModule::configure(injector);
    
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

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
} 