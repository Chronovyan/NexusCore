#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <fstream>
#include <filesystem>
#include "EditorDemoWindow.h"

// Error callback for GLFW
static void glfwErrorCallback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

int main(int argc, char** argv) {
    try {
        std::cout << "Starting EditorDemo application..." << std::endl;
        
        // Initialize GLFW
        std::cout << "Initializing GLFW..." << std::endl;
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW!" << std::endl;
            return -1;
        }
        
        // Create window with graphics context
        std::cout << "Creating GLFW window..." << std::endl;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        
        GLFWwindow* window = glfwCreateWindow(1280, 720, "AI-First Text Editor Demo", nullptr, nullptr);
        if (window == nullptr) {
            std::cerr << "Failed to create GLFW window!" << std::endl;
            glfwTerminate();
            return -1;
        }
        
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync
        
        // Initialize GLEW
        std::cout << "Initializing GLEW..." << std::endl;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            std::cerr << "Failed to initialize GLEW: " << glewGetErrorString(err) << std::endl;
            glfwTerminate();
            return -1;
        }
        
        // Setup ImGui context
        std::cout << "Setting up ImGui..." << std::endl;
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable keyboard controls
        
        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330");
        
        // Setup style
        ImGui::StyleColorsDark();
        
        // Create editor window
        std::cout << "Initializing editor window..." << std::endl;
        ai_editor::EditorDemoWindow editorWindow;
        editorWindow.initialize();
        
        // Check if a file was specified as an argument
        if (argc > 1) {
            std::string filename = argv[1];
            if (std::filesystem::exists(filename)) {
                std::cout << "Loading file: " << filename << std::endl;
                editorWindow.loadFile(filename);
            }
        } else {
            // Create new tab
            editorWindow.addNewTab();
        }
        
        std::cout << "Entering main loop. Press ESC to exit." << std::endl;
        
        // Main loop
        while (!glfwWindowShouldClose(window)) {
            // Poll and handle events
            glfwPollEvents();
            
            // Check for ESC key to exit
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, true);
            }
            
            // Start the ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            
            // Create a main menu bar
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("New", "Ctrl+N")) {
                        editorWindow.newFile();
                    }
                    if (ImGui::MenuItem("Open", "Ctrl+O")) {
                        editorWindow.openFile();
                    }
                    if (ImGui::MenuItem("Save", "Ctrl+S")) {
                        editorWindow.saveCurrentFile();
                    }
                    if (ImGui::MenuItem("Save As", "Ctrl+Shift+S")) {
                        editorWindow.saveFileAs();
                    }
                    ImGui::Separator();
                    if (ImGui::MenuItem("Exit", "Alt+F4")) {
                        glfwSetWindowShouldClose(window, true);
                    }
                    ImGui::EndMenu();
                }
                
                if (ImGui::BeginMenu("Edit")) {
                    if (ImGui::MenuItem("Undo", "Ctrl+Z", false, editorWindow.canUndo())) {
                        editorWindow.undo();
                    }
                    if (ImGui::MenuItem("Redo", "Ctrl+Y", false, editorWindow.canRedo())) {
                        editorWindow.redo();
                    }
                    ImGui::EndMenu();
                }
                
                ImGui::EndMainMenuBar();
            }
            
            // Render editor window
            bool open = true;
            editorWindow.render(&open);
            if (!open) {
                glfwSetWindowShouldClose(window, true);
            }
            
            // Rendering
            ImGui::Render();
            int displayW, displayH;
            glfwGetFramebufferSize(window, &displayW, &displayH);
            glViewport(0, 0, displayW, displayH);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            
            glfwSwapBuffers(window);
        }
        
        // Cleanup
        std::cout << "Cleaning up resources..." << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        
        glfwDestroyWindow(window);
        glfwTerminate();
        
        std::cout << "Application ended successfully." << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown error occurred." << std::endl;
        return -1;
    }
} 