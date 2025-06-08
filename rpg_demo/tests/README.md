# RPG Demo Tests

This directory contains the test suite for the RPG demo. The tests are written using the Google Test framework.

## Test Structure

- `test_rpgstate.cpp`: Tests for the core game state management
- `test_rpggame.cpp`: Tests for the game logic and AI integration

## Running Tests

### Prerequisites

- CMake 3.15 or higher
- C++17 compatible compiler
- Google Test (automatically downloaded by CMake)

### Building and Running Tests

1. Open a command prompt in the project root directory
2. Run the build script:
   ```
   .\build_and_test.bat
   ```

This will:
1. Configure the build with CMake
2. Build the project and tests
3. Run all tests
4. Show test results

### Running Individual Tests

After building, you can run individual test executables from the build directory:

```
cd build\tests\Debug
test_rpgstate.exe  # Run RPG state tests
test_rpggame.exe   # Run RPG game tests
```

## Writing New Tests

1. Create a new test file in this directory (e.g., `test_feature.cpp`)
2. Include the necessary headers and Google Test
3. Write test cases using the `TEST` or `TEST_F` macros
4. Add the new test file to `tests/CMakeLists.txt`
5. Rebuild and run the tests

## Test Naming Conventions

- Test files: `test_<component>.cpp`
- Test suites: `<Component>Test`
- Test cases: `TestedBehavior_ExpectedResult`

Example:
```cpp
TEST_F(PlayerTest, MovePlayer_UpdatesPosition) {
    // Test code here
}
```

## Debugging Tests

1. Open the generated Visual Studio solution in the `build` directory
2. Set the test executable as the startup project
3. Set breakpoints and debug as usual
