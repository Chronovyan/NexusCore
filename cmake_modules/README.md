# CMake Modules

This directory contains CMake modules for dependency management and build configuration.

## Files

- `Dependencies.cmake` - Manages project dependencies using vcpkg, Conan, or system packages.

## Usage

These modules are automatically included by the main CMakeLists.txt. Control dependency management with these CMake options:

```cmake
# Enable/disable package managers
option(ENABLE_VCPKG "Enable vcpkg for dependency management" ON)
option(ENABLE_CONAN "Enable Conan for dependency management" ON)

# Set paths to package managers if not in default locations
set(VCPKG_ROOT "path/to/vcpkg" CACHE PATH "Path to vcpkg installation")
set(CONAN_USER_HOME "path/to/conan/home" CACHE PATH "Path to Conan home directory")
```

## Adding Dependencies

### vcpkg
1. Add the package to the `vcpkg.json` file in the project root.
2. The package will be automatically installed by vcpkg.

### Conan
1. Add the package to `conanfile.txt` or `conanfile.py`.
2. The package will be installed by Conan during configuration.

### System Packages
For system packages, add `find_package()` calls to `Dependencies.cmake`.

## Platform-Specific Dependencies

Use platform checks in `Dependencies.cmake`:

```cmake
if(WIN32)
    # Windows-specific dependencies
elseif(APPLE)
    # macOS-specific dependencies
elseif(UNIX)
    # Linux/Unix-specific dependencies
endif()
```
