#include "AppDebugLog.h"
#include <iostream>
#include <string>
#include <cstdlib>
// Include GLEW before any other OpenGL-related headers
#include <GL/glew.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <regex>
#include "UIModel.h"
#include "OpenAI_API_Client.h"
#include "MockOpenAI_API_Client.h"
#include "WorkspaceManager.h"
#include "AIAgentOrchestrator.h"
#include "AIManager.h"
#include "tutorials/TutorialManager.hpp"
#include "tutorials/TutorialUIController.hpp"
#include "tutorials/TutorialProgressTracker.hpp"

// Include other necessary headers for your application 