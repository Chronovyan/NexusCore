# TextEditor (C++17)

A command-line C++17 text editor focused on stability and core editing features.

## Key Features & Status

- ✅ Core text buffer, cursor navigation, text manipulation (insert/delete)
- ✅ Selection, clipboard (copy/cut/paste), word operations
- ✅ File I/O (save/load)
- ✅ Search & Replace
- ✅ Automated testing framework (`ctest` / batch scripts)
- ⏳ Syntax highlighting (Planned)
- ⏳ Basic Graphical UI (Planned)

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

Alternative (Windows Batch for unit tests):
```batch
REM From project root
build_and_run_unit_tests.bat
```

## Basic Usage (CLI)

Launch the editor from its build location (e.g., `./build/src/TextEditor` or `bin\\TextEditor.exe`).
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

## Project Documentation

- **Core Principles & Practices:**
  - [`STABILITY.MD`](STABILITY.md): Editor stability principles.
  - [`THREAD_SAFETY.MD`](THREAD_SAFETY.md): Thread safety patterns.
  - [`REFINEMENTS.MD`](REFINEMENTS.md): C++ code refinements and best practices.
- **Development & Progress:**
  - [`ROADMAP.MD`](ROADMAP.md): Project development roadmap.
  - [`FUTURE_IMPROVEMENTS.MD`](FUTURE_IMPROVEMENTS.md): Ideas for future enhancements.
  - [`PROGRESS.MD`](PROGRESS.md): Key milestones achieved.
- **Features:**
  - [`docs/SearchFeature.md`](docs/SearchFeature.md): User guide for search/replace.
  - [`docs/SearchImplementationSummary.md`](docs/SearchImplementationSummary.md): Technical details of search.

## License

[MIT License](LICENSE)