# AI-First TextEditor

A modern text editor with integrated AI capabilities to help develop software projects.

## Features

- **AI-Powered Development**: Generate code, get suggestions, and automate routine tasks
- **Modern ImGui Interface**: Clean and intuitive user interface
- **OpenAI Integration**: Connect to OpenAI's API to leverage advanced AI models
- **Project Management**: Manage files and structure with AI assistance

## Requirements

- C++17 compatible compiler (MSVC, GCC, Clang)
- CMake 3.10+
- Git
- OpenAI API key

## Building from Source

### Windows (Visual Studio)
```powershell
# Clone the repository
git clone https://github.com/your-org/AI-First-TextEditor.git
cd AI-First-TextEditor

# Create build directory and generate Visual Studio solution
mkdir build_vs
cd build_vs
cmake -G "Visual Studio 17 2022" -A x64 ..

# Build using CMake
cmake --build . --config Release
```

### Linux/macOS
```bash
# Clone the repository
git clone https://github.com/your-org/AI-First-TextEditor.git
cd AI-First-TextEditor

# Create build directory and generate Makefiles
mkdir build
cd build
cmake ..

# Build
make -j$(nproc)
```

## Environment Setup
Create a `.env` file in the executable directory:
```
OPENAI_API_KEY=your_api_key_here
OPENAI_ORGANIZATION=your_org_id_here  # Optional
```

You can also copy and edit the provided `env.template` file.

## Usage

1. Launch the application
2. Configure your OpenAI API key in Settings if not using .env
3. Start creating your project with AI assistance

## Directory Structure

- `src/`: Source code for the editor
- `tests/`: Unit and integration tests
- `external/`: External dependencies
- `docs/`: Documentation
- `build_vs/`: Visual Studio build directory
- `build/`: Default build directory

## Testing

Run all tests:
```bash
cd build_vs
.\tests\RunAllTests.exe
```

## Documentation

Additional documentation is available in the [docs/](docs/) directory.

## License

[MIT License](LICENSE) 