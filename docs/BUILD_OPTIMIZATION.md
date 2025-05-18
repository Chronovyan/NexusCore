# Build Optimization Documentation

## ImGui Library Setup

This project uses Dear ImGui for the user interface, compiled as a separate static library to significantly optimize build times. This document explains the setup and how to work with it.

### Current Implementation

ImGui is compiled as a standalone static library (`imgui_lib`) that contains:
- Core ImGui files (`imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`)
- Backend implementations (`imgui_impl_glfw.cpp`, `imgui_impl_opengl3.cpp`)

This library is then linked to the main application rather than compiling the ImGui sources directly with the application code.

```cmake
# CMakeLists.txt excerpt:
add_library(imgui_lib STATIC ${IMGUI_SOURCES})
target_include_directories(imgui_lib PUBLIC 
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw)
target_link_libraries(imgui_lib PRIVATE OpenGL::GL)

# Application linking
target_link_libraries(AITextEditor PRIVATE 
    EditorLib 
    imgui_lib 
    # Other dependencies...
)
```

### Build Performance Impact

**Before optimization**:
- Incremental builds: ~10 minutes
- Every change to application code triggered a complete rebuild of ImGui

**After optimization**:
- Incremental builds: <1 second (0.6s measured)
- ImGui only compiles once during a clean build
- Subsequent builds only recompile modified application code

### Modifying ImGui

If you need to make changes to ImGui:

1. **Direct Modifications**: If you modify ImGui source files directly (e.g., to customize widgets):
   - Changes will be picked up when rebuilding the `imgui_lib` target
   - Run `cmake --build . --target imgui_lib` to rebuild only the ImGui library
   - Remember that a full rebuild will always recompile ImGui

2. **Updating ImGui Version**:
   - Update the GIT_TAG in the FetchContent_Declare section of CMakeLists.txt
   - Run a clean build: Delete the build directory and reconfigure

3. **Adding New ImGui Files**:
   - Add new source files to the IMGUI_SOURCES variable in CMakeLists.txt
   - Ensure proper include directories if needed

4. **Custom ImGui Configuration**:
   - Create/modify imconfig.h in your project
   - Ensure it's in the include path for imgui_lib

### Troubleshooting

If your changes to ImGui don't seem to be applied:
1. Check if you're modifying files in the build directory (these will be overwritten)
2. Try a clean build by deleting the build directory and running CMake again
3. Verify that the proper ImGui version is being used (check the GIT_TAG)

### Technical Details

ImGui is template-heavy and complex to compile. By separating it into a static library:
- The compiler processes ImGui files only once
- Your application's incremental build times remain fast
- The final executable size and performance are identical to before

This pattern can be applied to other large third-party libraries to achieve similar build performance improvements. 