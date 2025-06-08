#ifndef TEST_DECLARATIONS_H
#define TEST_DECLARATIONS_H

#include "TestFramework.h"

// Command Manager tests (CommandManagerTests.cpp)
TestResult testCommandManager_ExecuteUndoRedo();
TestResult testCommandManager_RedoStackClearing();

// Comprehensive Undo/Redo tests (ComprehensiveUndoRedoTest.cpp)
TestResult testUndoRedoBasicAddLines();
TestResult testUndoRedoTextInsertion();
TestResult testUndoRedoDeletion();
TestResult testUndoRedoNewLine();
TestResult testRedoStackClearing();
TestResult testComplexUndoRedoSequence();

// Selection and clipboard tests (SelectionClipboardTest.cpp)
TestResult testBasicSelection();
TestResult testClipboardOperations();
TestResult testMultiLineSelection();
TestResult testWordSelection();
TestResult testDeleteWord();

// Exit tests (ExitTest.cpp)
TestResult testExitCommands();
TestResult testExitWithUnsavedChanges();

// Simple DI tests (SimpleDITest.cpp)
TestResult testSimpleDI_GetInterface();
TestResult testSimpleDI_ResolveInterface();
TestResult testSimpleDI_DependencyChain();
TestResult testSimpleDI_MixedRegistrationStyles();
TestResult testSimpleDI_ErrorHandlingUnregisteredTypes();

// Standalone DI tests (StandaloneDITest.cpp)
TestResult testStandaloneDI_BasicResolution();
TestResult testStandaloneDI_ModernInterface();
TestResult testStandaloneDI_LegacyInterface();
TestResult testStandaloneDI_MixedStyles();

// Lifetime Manager tests (LifetimeManagerTest.cpp)
TestResult testLifetimeManager_SingletonLifetime();
TestResult testLifetimeManager_TransientLifetime();
TestResult testLifetimeManager_ScopedLifetime();
TestResult testLifetimeManager_ThreadSafety();
TestResult testLifetimeManager_LifetimeInjector();

#endif // TEST_DECLARATIONS_H 