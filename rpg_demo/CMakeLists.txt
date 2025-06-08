cmake_minimum_required(VERSION 3.15)
project(AITextRpgDemo)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Function to find and copy DLLs to output directory
function(copy_dlls target_name)
    if(WIN32)
        # Find all DLLs that the target depends on
        get_target_property(dlls ${target_name} LINK_LIBRARIES)
        
        # Create a custom command to copy DLLs
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target_name}>/../dlls"
            COMMENT "Creating DLLs directory for ${target_name}"
        )
        
        # Copy each DLL to the output directory
        foreach(dll ${dlls})
            if(TARGET ${dll})
                get_target_property(dll_path ${dll} IMPORTED_LOCATION)
                if(dll_path AND EXISTS "${dll_path}")
                    add_custom_command(TARGET ${target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy "${dll_path}" "$<TARGET_FILE_DIR:${target_name}>/../dlls/"
                        COMMENT "Copying ${dll} to output directory"
                    )
                endif()
            endif()
        endforeach()
    endif()
endfunction()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# Instead of using add_subdirectory, we'll create an imported library for EditorLib
# First, build the EditorLib target in its own build directory
set(EDITORLIB_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/editorlib-build")
set(EDITORLIB_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/editorlib-install")

# Create build and install directories
file(MAKE_DIRECTORY ${EDITORLIB_BUILD_DIR})
file(MAKE_DIRECTORY ${EDITORLIB_INSTALL_DIR})

# Configure and build EditorLib
execute_process(
    COMMAND ${CMAKE_COMMAND} -G ${CMAKE_GENERATOR} -A ${CMAKE_GENERATOR_PLATFORM}
        -DCMAKE_INSTALL_PREFIX=${EDITORLIB_INSTALL_DIR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DINCLUDE_GOOGLETEST=OFF
        -DBUILD_TESTING=OFF
        ${CMAKE_SOURCE_DIR}/..
    WORKING_DIRECTORY ${EDITORLIB_BUILD_DIR}
    RESULT_VARIABLE CONFIGURE_RESULT
)

if(NOT CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to configure EditorLib: ${CONFIGURE_RESULT}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --config ${CMAKE_BUILD_TYPE} --target install
    WORKING_DIRECTORY ${EDITORLIB_BUILD_DIR}
    RESULT_VARIABLE BUILD_RESULT
)

if(NOT BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to build EditorLib: ${BUILD_RESULT}")
endif()

# Add the installed EditorLib to the build
add_library(EditorLib STATIC IMPORTED)
set_target_properties(EditorLib PROPERTIES
    IMPORTED_LOCATION ${EDITORLIB_INSTALL_DIR}/lib/EditorLib.lib
    INTERFACE_INCLUDE_DIRECTORIES ${EDITORLIB_INSTALL_DIR}/include
)

# Add the include directory for EditorLib
target_include_directories(EditorLib INTERFACE ${EDITORLIB_INSTALL_DIR}/include)

# Add alias for the imported target
add_library(TextEditor::EditorLib ALIAS EditorLib)

# Add main executable
add_executable(ai_text_rpg
    main.cpp
    RPGState.cpp
    RPGGame.cpp
)

# Link libraries for main executable
target_link_libraries(ai_text_rpg PRIVATE
    TextEditor::EditorLib  # Link with the main editor library
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
)

# Add the main project's binary directory to the include path
target_include_directories(ai_text_rpg PRIVATE
    ${CMAKE_SOURCE_DIR}/../src
)

# Copy required DLLs for main executable
copy_dlls(ai_text_rpg)

# Set output directory for main executable
set_target_properties(ai_text_rpg PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    VS_DEBUGGER_ENVIRONMENT "PATH=${CMAKE_BINARY_DIR}/bin;${CMAKE_BINARY_DIR}/dlls;%PATH%"
)

# Copy assets
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/assets)
file(GLOB_RECURSE ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
foreach(ASSET ${ASSETS})
    file(RELATIVE_PATH RELATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${ASSET})
    get_filename_component(ASSET_DIR ${RELATIVE_PATH} DIRECTORY)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/assets/${ASSET_DIR})
    add_custom_command(
        TARGET ai_text_rpg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${ASSET} ${CMAKE_BINARY_DIR}/bin/assets/${RELATIVE_PATH}
    )
endforeach()

# Add tests
add_subdirectory(tests)
