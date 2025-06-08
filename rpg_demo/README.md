# AI-Assisted Text RPG Demo

This is a simple text-based RPG game that demonstrates the AI-assisted text editing capabilities of the AI-First TextEditor. The game features:

- A simple text-based interface
- AI-generated content (descriptions, NPC responses, etc.)
- Basic inventory and world interaction
- Extensible architecture for adding more features

## Prerequisites

- Windows 10/11 or Linux/macOS (Windows recommended for best experience)
- [Git](https://git-scm.com/downloads)
- [CMake](https://cmake.org/download/) 3.15 or higher
- C++17 compatible compiler (Visual Studio 2022 recommended on Windows, GCC/Clang on Linux/macOS)
- [vcpkg](https://vcpkg.io/en/getting-started.html) - C++ Library Manager

## Quick Start (Windows)

1. **Clone the repository** (if you haven't already):
   ```
   git clone https://github.com/yourusername/ai-first-texteditor.git
   cd ai-first-texteditor/rpg_demo
   ```

2. **Run the setup script**:
   Open PowerShell and run:
   ```powershell
   .\setup_environment.ps1
   ```
   This will:
   - Set up the build environment
   - Configure the project with CMake
   - Build the project and all dependencies
   - Run the tests

3. **Run the game**:
   ```
   .\build\bin\Debug\ai_text_rpg.exe
   ```

## Manual Build Instructions

### 1. Install vcpkg

If you don't have vcpkg installed:

```powershell
# From the directory where you want to install vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat  # On Windows
# or
# ./bootstrap-vcpkg.sh  # On Linux/macOS
```

### 2. Install Dependencies

```powershell
vcpkg install gtest:x64-windows  # For testing
# For development with Visual Studio, also install:
vcpkg integrate install
```

### 3. Build the Project

```powershell
# Create and navigate to build directory
mkdir build
cd build

# Configure with CMake
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE="path/to/your/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DCMAKE_BUILD_TYPE=Debug

# Build the project
cmake --build . --config Debug
```

### 4. Run Tests

```powershell
# Run all tests
ctest -C Debug --output-on-failure

# Or run a specific test
.\tests\Debug\test_rpgstate.exe
```

## Project Structure

- `src/` - Main source code
- `tests/` - Unit tests
- `assets/` - Game assets (if any)
- `build/` - Build directory (created during build)
  - `bin/` - Compiled executables
  - `dlls/` - Required DLLs (Windows)

## Development

### IDE Setup

#### Visual Studio Code

1. Install the following extensions:
   - C/C++
   - CMake Tools
   - Test Explorer UI
   - Google Test Adapter

2. Open the project folder in VS Code

3. Select the CMake kit (Ctrl+Shift+P -> "CMake: Select a Kit")

4. Build the project (Ctrl+Shift+P -> "CMake: Build")

#### Visual Studio

1. Open the `CMakeLists.txt` as a project

2. Select the desired build configuration (Debug/Release)

3. Build the solution (F7)

## Game Commands

- `look` or `l` - Look around the current location
- `go <direction>` - Move in a direction (north, south, east, west, etc.)
- `get <item>` - Pick up an item
- `drop <item>` - Drop an item
- `inventory` or `i` - Check your inventory
- `examine <object>` or `x <object>` - Examine an object
- `help` - Show available commands
- `quit` or `q` - Quit the game
- `help` - Show available commands
- `quit` or `q` - Quit the game

## AI Integration

The demo includes stubs for AI integration. To enable full AI content generation, you'll need to:

1. Set up an OpenAI API key
2. Implement the `initializeAIManager()` function in `main.cpp`
3. Rebuild the project

## Extending the Game

You can extend the game by:

1. Adding new locations in `RPGGame::loadDefaultWorld()`
2. Creating new command handlers in `RPGState.cpp`
3. Adding new game objects and interactions
4. Enhancing the AI integration for more dynamic content generation

## License

This project is part of the AI-First TextEditor and is licensed under the same terms.
