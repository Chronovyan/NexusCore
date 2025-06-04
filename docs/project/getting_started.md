# Getting Started with AI-First TextEditor

This guide will help you set up the AI-First TextEditor for development.

## First-Time Setup

### Prerequisites

Before you begin, ensure you have:

- C++ compiler with C++17 support (MSVC, GCC, or Clang)
- CMake 3.10 or higher
- Git
- OpenAI API key

### Obtain an OpenAI API Key

1. Sign up or log in at [OpenAI Platform](https://platform.openai.com/)
2. Navigate to "API Keys" section
3. Create a new API key
4. Save the key securely - you'll need it for the application

### Building the Application

#### Windows (Visual Studio)

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

#### Linux/macOS

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

### Configure API Access

1. Create a `.env` file in the executable directory with your API key:

```
OPENAI_API_KEY=your_api_key_here
```

2. Alternatively, you can copy the `env.template` file and fill in your key:

```bash
cp env.template .env
# Edit the .env file with your editor
```

### Running the Application

Navigate to your build directory and run the executable:

```bash
# Windows
.\TextEditor.exe

# Linux/macOS
./TextEditor
```

## Creating Your First Project

1. Launch the application
2. Click "New Project" in the welcome screen
3. Enter a project name and select a directory
4. Type a description of what you want to build in the prompt area
5. Let the AI generate an initial project plan
6. Review and approve the plan
7. Watch as the AI generates files for your project
8. Edit the files as needed using the built-in editor

## Next Steps

- Check out the [Codebase Structure](development/CODEBASE_STRUCTURE.md) document to learn more about the architecture
- See [Testing Guide](testing_guide.md) to learn about running and writing tests
- Review [OpenAI Integration](features/ai_integration.md) to understand how the AI capabilities work 