# TextEditor

A C++17 text editor implementation with support for basic text editing operations.

## Features

- Text buffer management with line-based operations
- Cursor movement (basic and advanced navigation)
- Text insertion, deletion, and manipulation
- Selection support with clipboard operations (copy, cut, paste)
- Word-based operations
- File I/O (save/load)
- Command-line interface

## Current Implementation Status

- ✅ Core text buffer implementation
- ✅ Basic cursor navigation
- ✅ Advanced cursor movement (line start/end, word jumping)
- ✅ Text selection and clipboard operations
- ✅ File I/O operations
- ⏳ Syntax highlighting (planned)
- ⏳ Search and replace (planned)
- ⏳ Graphical UI (planned)

## Building from Source

### Prerequisites

- C++17 compatible compiler (MSVC, GCC, Clang)

### Build Instructions

Using the included batch build system:

```
build.bat
```

## Usage

The text editor currently provides a command-line interface with the following commands:

### Basic Buffer Operations
| Command | Description |
|---------|-------------|
| `add <text>` | Adds text as a new line at the end |
| `insert <index> <text>` | Inserts text at the given 0-based line index |
| `delete <index>` | Deletes the line at the given 0-based index |
| `replace <index> <text>` | Replaces the line at the given 0-based index |
| `view` | Prints the entire buffer with cursor position |
| `lines` | Shows the current number of lines |
| `clear` | Clears all lines from the buffer |

### Cursor Movement
| Command | Description |
|---------|-------------|
| `cursor` | Shows current cursor position |
| `setcursor <line> <col>` | Sets cursor to specified position |
| `cu` | Move cursor up |
| `cd` | Move cursor down |
| `cl` | Move cursor left |
| `cr` | Move cursor right |
| `home` | Move cursor to start of line |
| `end` | Move cursor to end of line |
| `top` | Move cursor to start of buffer |
| `bottom` | Move cursor to end of buffer |
| `nextword` | Move cursor to next word |
| `prevword` | Move cursor to previous word |

### Text Editing
| Command | Description |
|---------|-------------|
| `type <text>` | Inserts text at the cursor position |
| `backspace` | Deletes the character before the cursor |
| `del` | Deletes the character at the cursor position |
| `newline` | Inserts a line break at the cursor position |
| `join` | Joins the current line with the next line |
| `delword` | Deletes word at cursor position |

### Selection and Clipboard
| Command | Description |
|---------|-------------|
| `selstart` | Starts text selection at current cursor position |
| `selend` | Ends text selection at current cursor position |
| `selclear` | Clears current selection |
| `selshow` | Shows selected text |
| `selword` | Selects word at cursor position |
| `cut` | Cuts selected text to clipboard |
| `copy` | Copies selected text to clipboard |
| `paste` | Pastes clipboard content at cursor position |

### File Operations
| Command | Description |
|---------|-------------|
| `save <filename>` | Saves the buffer content to a file |
| `load <filename>` | Loads content from a file |

### Miscellaneous
| Command | Description |
|---------|-------------|
| `help` | Shows help message |
| `quit` or `exit` | Exits the editor |

## License

[MIT License](LICENSE) 