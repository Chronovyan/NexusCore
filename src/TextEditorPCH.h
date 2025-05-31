#pragma once

// Standard library includes
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <cassert>
#include <cmath>
#include <utility>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <optional>
#include <type_traits>

// Project-specific includes
#include "Editor.h"
#include "TextBuffer.h"
#include "Command.h"
#include "CommandManager.h"
#include "EditorCommands.h"
#include "EditorError.h"

// Comment to prevent trailing newline issues 