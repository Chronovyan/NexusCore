# Contributing to AI-First Text Editor

Thank you for your interest in contributing to AI-First Text Editor! We welcome contributions from everyone.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [Code Style](#code-style)
- [Pull Request Process](#pull-request-process)
- [Reporting Bugs](#reporting-bugs)
- [Feature Requests](#feature-requests)
- [License](#license)

## Code of Conduct

This project adheres to the [Contributor Covenant](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code.

## Getting Started

1. Fork the repository on GitHub
2. Clone your fork locally
3. Set up the development environment (see below)
4. Create a feature branch (`git checkout -b feature/amazing-feature`)
5. Make your changes
6. Test your changes
7. Commit your changes following the [commit guidelines](#commit-guidelines)
8. Push to your fork (`git push origin feature/amazing-feature`)
9. Open a Pull Request

## Development Environment

### Prerequisites

- Windows 10/11, macOS, or Linux
- Git
- CMake 3.21+
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- Python 3.8+
- vcpkg (for dependency management)

### Setting Up

#### Windows

```powershell
# Run as administrator
Set-ExecutionPolicy Bypass -Scope Process -Force
.\scripts\setup_dev.ps1
```

#### Linux/macOS

```bash
# Install dependencies
sudo apt-get update && sudo apt-get install -y git cmake build-essential python3-pip

# Clone the repository
git clone https://github.com/yourusername/ai-text-editor.git
cd ai-text-editor

# Set up Python packages
pip install -r requirements-dev.txt

# Set up vcpkg
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh

# Configure and build
mkdir -p build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

## Code Style

### C++

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Use `clang-format` for formatting
- Maximum line length: 100 characters
- Indentation: 4 spaces (no tabs)
- Braces: Attached style

### Python

- Follow [PEP 8](https://www.python.org/dev/peps/pep-0008/)
- Use `black` for formatting
- Maximum line length: 100 characters
- Use type hints for all function signatures

### Commit Guidelines

We follow the [Conventional Commits](https://www.conventionalcommits.org/) specification:

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

#### Types

- `feat`: A new feature
- `fix`: A bug fix
- `docs`: Documentation only changes
- `style`: Changes that do not affect the meaning of the code
- `refactor`: A code change that neither fixes a bug nor adds a feature
- `perf`: A code change that improves performance
- `test`: Adding missing tests or correcting existing tests
- `build`: Changes that affect the build system or external dependencies
- `ci`: Changes to CI configuration files and scripts
- `chore`: Other changes that don't modify src or test files

#### Examples

```
feat(editor): add multi-cursor support

Adds the ability to create and manipulate multiple cursors in the editor.
Closes #123
```

```
fix(core): prevent null pointer dereference in text buffer

Fixes an issue where a null pointer could be dereferenced when handling
empty lines in the text buffer.

Fixes #456
```

## Pull Request Process

1. Ensure any install or build dependencies are removed before the end of the layer when doing a build
2. Update the README.md with details of changes to the interface
3. Increase the version numbers in any examples files and the README.md to the new version that this Pull Request would represent
4. You may merge the Pull Request once you have the sign-off of two other developers, or if you do not have permission to do that, you may request the second reviewer to merge it for you

## Reporting Bugs

Please use the [GitHub issue tracker](https://github.com/yourusername/ai-text-editor/issues) to report bugs. Include the following information:

1. A clear and descriptive title
2. Steps to reproduce the issue
3. Expected behavior
4. Actual behavior
5. Screenshots if applicable
6. Your operating system and version
7. Any other relevant information

## Feature Requests

We welcome feature requests! Please open an issue on the [GitHub issue tracker](https://github.com/yourusername/ai-text-editor/issues) with the following information:

1. A clear and descriptive title
2. A description of the feature
3. Why this feature would be useful
4. Any examples or mockups if applicable

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](LICENSE).
