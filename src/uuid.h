#pragma once

#include <string>
#include <random>
#include <sstream>
#include <iomanip>

/**
 * @brief Simple UUID generation functions
 * 
 * This is a lightweight mock implementation of UUID generation for the codebase indexer.
 * In a production environment, you would use a proper UUID library like libuuid or boost::uuid.
 */
namespace uuid {

/**
 * @brief Generate a random UUID v4
 * 
 * @return std::string The generated UUID as a string
 */
inline std::string generate_uuid_v4() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream ss;
    ss << std::hex;

    for (int i = 0; i < 8; i++) {
        ss << dis(gen);
    }
    ss << "-";

    for (int i = 0; i < 4; i++) {
        ss << dis(gen);
    }
    ss << "-4";  // Version 4 UUID

    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";
    
    ss << dis2(gen);  // Variant bits
    
    for (int i = 0; i < 3; i++) {
        ss << dis(gen);
    }
    ss << "-";

    for (int i = 0; i < 12; i++) {
        ss << dis(gen);
    }

    return ss.str();
}

} // namespace uuid 