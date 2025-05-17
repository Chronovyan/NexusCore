# TextEditor (C++17)

A command-line C++17 text editor focused on stability, performance, and core editing features.

## Key Features & Status

- ✅ Core text buffer, cursor navigation, text manipulation (insert/delete)
- ✅ Selection management with semantic units (words, lines, expressions, blocks, etc.)
- ✅ Clipboard operations (copy/cut/paste) with robust memory management
- ✅ File I/O (open/save) with modification tracking
- ✅ Search & Replace with case sensitivity options
- ✅ Indentation management (increase/decrease)
- ✅ Undo/redo functionality with efficient command history
- 🔄 Syntax highlighting for C++ (stable with ongoing optimizations)
- ⏳ Basic Graphical UI (Planned)

## AI-First TextEditor Pivot

We've pivoted to an AI-First approach where the textual editor serves as the foundation for an intelligent coding environment:

- 🔄 Dear ImGui-based user interface with conversation view, file sidebar, and status display
- 🔄 OpenAI API integration for AI-powered code generation and assistance
- 🔄 AI orchestration system managing the conversation flow and development process
- ⏳ Project generation from natural language descriptions
- ⏳ Compilation, testing, and execution pipeline for generated code

This "AI-First" approach reimagines the editor as an environment where users provide high-level guidance while the AI handles implementation details.

## Building & Running

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake (version 3.10+ recommended)

### Build Instructions

Primary method (CMake):
```bash
# From project root
mkdir build && cd build
cmake ..
cmake --build . # Or: make / msbuild TextEditor.sln (depending on generator)
# Executable typically in: build/src/TextEditor, build/bin/TextEditor, or build/TextEditor
```
(For more detailed instructions, see [docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md).)

Alternative (Windows Batch for building and testing):
```batch
REM From project root - ensures clean build and runs tests
build_run_test.bat
REM Executable in: bin/TextEditor.exe (if produced by this script)
```
For just building with batch scripts, see `build.bat`.

### Running Tests

Using CTest (after CMake build, from `build` directory):
```bash
ctest -C Debug # Or Release, etc.
```
(For more detailed instructions, see [docs/BUILD_AND_TEST.md](docs/BUILD_AND_TEST.md).)

Alternative (Windows Batch for unit tests):
```batch
REM From project root
build_and_run_unit_tests.bat
```

## Basic Usage (CLI)

Launch the editor from its build location (e.g., `./build/src/TextEditor` or `bin\TextEditor.exe`).

It provides an interactive command prompt. Key commands:

| Command                                      | Description                                      |
| -------------------------------------------- | ------------------------------------------------ |
| `load <filename>`                            | Load file into buffer.                           |
| `save <filename>`                            | Save buffer to file.                             |
| `add <text>`                                 | Add text as a new line at the end.               |
| `insert <line_idx> <text>`                   | Insert text at specified 0-based line index.     |
| `delete <line_idx>`                          | Delete line at 0-based index.                    |
| `view`                                       | Print buffer with cursor.                        |
| `setcursor <line> <col>`                     | Set cursor position.                             |
| `type <text>`                                | Insert text at cursor.                           |
| `search <term>` / `find <term>`              | Find text.                                       |
| `replace <search_term> <replace_text>`       | Find and replace first occurrence.               |
| `replaceall <search_term> <replace_text>`    | Replace all occurrences.                         |
| `help`                                       | Show all commands.                               |
| `quit` / `exit`                              | Exit the editor.                                 |

(Use the `help` command for a comprehensive list of all available operations.)

## Selection Units

The editor supports semantic selection units for intelligent text selection:

- Character: Individual character selection
- Word: Word-level selection (alphanumeric + underscore)
- Expression: Expression-level selection (e.g., function call, parenthesized expression)
- Line: Line-level selection (entire line)
- Paragraph: Paragraph-level selection (consecutive non-empty lines)
- Block: Block-level selection (code blocks in curly braces)
- Document: Entire document selection

Selections can be expanded or shrunk between these units, providing intuitive text manipulation.

## Project Architecture

The project follows a modular architecture with clean separation between components:

- **Text Buffer Layer**: Manages the storage and manipulation of text content
- **Command Layer**: Implements the command pattern for all operations with undo/redo support
- **Editor Layer**: Provides the high-level API for text editing operations
- **Syntax Highlighting Layer**: Handles language-specific syntax highlighting

For a detailed breakdown of the project structure, features, and API, see [docs/EDITOR_OUTLINE.md](docs/EDITOR_OUTLINE.md).

## Project Documentation

- **Core Principles & Practices:**
  - [`docs/STABILITY.MD`](docs/STABILITY.md): Editor stability principles.
  - [`docs/THREAD_SAFETY.MD`](docs/THREAD_SAFETY.md): Thread safety patterns.
  - [`docs/REFINEMENTS.MD`](docs/REFINEMENTS.md): C++ code refinements and best practices.
  - [`docs/SYNTAX_HIGHLIGHTING_INVESTIGATION.MD`](docs/SYNTAX_HIGHLIGHTING_INVESTIGATION.md): Technical investigation of the syntax highlighting manager.
  - [`docs/SYNTAX_HIGHLIGHTING_FIXES.MD`](docs/SYNTAX_HIGHLIGHTING_FIXES.md): Recent fixes to the syntax highlighting system.
- **Development & Progress:**
  - [`ROADMAP.MD`](ROADMAP.md): Project development roadmap.
  - [`docs/FUTURE_IMPROVEMENTS.MD`](docs/FUTURE_IMPROVEMENTS.md): Ideas for future enhancements.

## License

[MIT License](LICENSE)