# Phase 1 Implementation Plan: Foundational Stability, Cleanup, and Standardization

This document outlines the specific implementation steps for Phase 1 of our improvement roadmap, with a focus on setting up a solid foundation for future AI integration.

## 1. Remove Test-Specific Code

### 1.1. Editor.cpp - `deleteWord()` Method
- Remove the special handling for the test case with "The quick brown fox jumps over the lazy dog."
- Update related tests to use proper mocking or setup instead of relying on hardcoded conditions

### 1.2. EditorCommands.cpp - `fixDeleteWordForTest` Function
- Remove this helper function entirely
- Update any tests that rely on it to use proper test fixtures or mocks

### 1.3. EditorCommands.cpp - Special Conditions in Command Implementations
- Remove any hardcoded test conditions in `InsertTextCommand::execute`, `ReplaceCommand::execute`, etc.
- Ensure commands follow a consistent pattern of execution and undo

### 1.4. SyntaxHighlighter.cpp - Special Cases in `CppHighlighter::highlightLine`
- Remove special case handling for test strings like "myVariable anotherVar" and "int x = 1; /* start comment"
- Update tests to properly set up the state instead of relying on special case handling

## 2. Standardize Error Handling in TextBuffer.cpp

### 2.1. Review and Standardize Error Handling Methods
- Decide on consistent approach: throw exceptions for out-of-range access
- Update the following methods to use consistent error handling:
  - `deleteLine`: Change from silent failure to throwing exceptions
  - `replaceLine`: Change from silent failure to throwing exceptions
  - Review other methods for consistent behavior

### 2.2. Document Error Handling Behavior
- Add clear comments about expected behavior and error conditions
- Update class documentation to explain the error handling strategy

## 3. Fix InsertArbitraryTextCommand::undo

### 3.1. Implement Correct Undo Logic
- Replace character-by-character undo with range deletion
- Correctly handle newlines in the text that was inserted
- Use `directDeleteTextRange` if available, or implement proper range functionality

### 3.2. Add Tests to Verify Undo Behavior
- Create tests specifically for multi-line insertions and verifying undo

## 4. Standardize Logging

### 4.1. Review and Clean Up `logManagerMessage` in SyntaxHighlightingManager.cpp
- Simplify conditional logic around logging flags
- Consider using compile-time flags for test-specific behavior

### 4.2. Create a Consistent Logging Interface
- Replace `std::cout` and `std::cerr` debug messages with `ErrorReporter`
- Define and use appropriate severity levels consistently

### 4.3. Update CppHighlighter Debugging
- Remove or properly gate debug print statements behind logging configuration

## 5. Address DecreaseIndentCommand Selection Logic

### 5.1. Review and Fix Selection Logic
- Clarify and fix the indentation logic when a selection is active
- Ensure it correctly unindents all selected lines
- Maintain proper selection state after operation

### 5.2. Add Tests for Selection-Based Indentation
- Create specific tests for decreasing indentation with active selections

## 6. Review DeleteCharCommand Role

### 6.1. Define Clear Purpose for DeleteCharCommand
- Clarify if it's a user-facing command or low-level utility
- Consider relationship with BackspaceCommand and ForwardDeleteCommand

### 6.2. Refactor or Remove If Redundant
- If it's redundant, update callers to use more specific commands
- If it's a utility, ensure it's properly documented and not exposed in UI

## 7. Define Clear APIs for Buffer/Editor State Access

### 7.1. Review and Enhance TextBuffer API
- Ensure consistent accessor methods for buffer content
- Add missing methods if needed for clear content access
- Make accessor methods const-correct

### 7.2. Define Editor API for State Access
- Review methods for accessing cursor, selection, file type
- Add any missing methods needed for external components
- Document these APIs clearly for future AI integration

## Implementation Strategy

1. **One Task at a Time**: Implement changes for each section methodically
2. **Test-Driven Approach**: 
   - Write or update tests first for each change
   - Verify that modified code passes the tests
3. **Incremental Commits**: Make small, focused commits with clear messages
4. **Documentation**: Add/update comments explaining:
   - Why the code exists (not just what it does)
   - Error handling expectations
   - API usage information
5. **Backward Compatibility**: Consider impact on existing code and tests

## Success Criteria

- All test-specific code is removed from production code
- Error handling is consistent and well-documented
- Command undo operations work correctly, especially for multi-line operations
- Logging is standardized and configurable
- APIs for key operations are clear and consistent
- All tests pass after the changes

This implementation plan ensures we build a solid foundation before moving on to more complex architectural improvements or AI integration. 