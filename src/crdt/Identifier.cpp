#include "crdt/Identifier.hpp"
#include <algorithm>
#include <cstdlib>
#include <random>
#include <nlohmann/json.hpp>

namespace ai_editor {

using json = nlohmann::json;

// Maximum value for identifier digits
const uint32_t MAX_DIGIT_VALUE = 0xFFFFFF;

// IdentifierElement implementation
IdentifierElement::IdentifierElement(uint32_t digit, const std::string& clientId)
    : digit_(digit), clientId_(clientId) {}

uint32_t IdentifierElement::getDigit() const {
    return digit_;
}

std::string IdentifierElement::getClientId() const {
    return clientId_;
}

int IdentifierElement::compareTo(const IdentifierElement& other) const {
    if (digit_ < other.digit_) {
        return -1;
    }
    if (digit_ > other.digit_) {
        return 1;
    }
    
    // Same digit, compare client IDs
    if (clientId_ < other.clientId_) {
        return -1;
    }
    if (clientId_ > other.clientId_) {
        return 1;
    }
    
    return 0; // Equal
}

bool IdentifierElement::operator==(const IdentifierElement& other) const {
    return compareTo(other) == 0;
}

bool IdentifierElement::operator<(const IdentifierElement& other) const {
    return compareTo(other) < 0;
}

// Identifier implementation
Identifier::Identifier() {}

Identifier::Identifier(const std::vector<IdentifierElement>& elements)
    : elements_(elements) {}

const std::vector<IdentifierElement>& Identifier::getElements() const {
    return elements_;
}

int Identifier::compareTo(const Identifier& other) const {
    // Compare elements one by one
    size_t minSize = std::min(elements_.size(), other.elements_.size());
    
    for (size_t i = 0; i < minSize; ++i) {
        int comp = elements_[i].compareTo(other.elements_[i]);
        if (comp != 0) {
            return comp;
        }
    }
    
    // If all shared elements are equal, the shorter path is less
    if (elements_.size() < other.elements_.size()) {
        return -1;
    }
    if (elements_.size() > other.elements_.size()) {
        return 1;
    }
    
    return 0; // Equal
}

bool Identifier::operator==(const Identifier& other) const {
    return compareTo(other) == 0;
}

bool Identifier::operator<(const Identifier& other) const {
    return compareTo(other) < 0;
}

std::string Identifier::toJson() const {
    json j;
    json elements = json::array();
    
    for (const auto& element : elements_) {
        elements.push_back({
            {"digit", element.getDigit()},
            {"clientId", element.getClientId()}
        });
    }
    
    j["elements"] = elements;
    return j.dump();
}

Identifier Identifier::fromJson(const std::string& jsonStr) {
    json j = json::parse(jsonStr);
    
    std::vector<IdentifierElement> elements;
    if (j.contains("elements") && j["elements"].is_array()) {
        for (const auto& elementJson : j["elements"]) {
            uint32_t digit = elementJson["digit"].get<uint32_t>();
            std::string clientId = elementJson["clientId"].get<std::string>();
            elements.emplace_back(digit, clientId);
        }
    }
    
    return Identifier(elements);
}

Identifier Identifier::create(const std::string& clientId) {
    // Create a new identifier with a single random element
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(1, MAX_DIGIT_VALUE - 1);
    
    std::vector<IdentifierElement> elements;
    elements.emplace_back(dist(gen), clientId);
    
    return Identifier(elements);
}

Identifier Identifier::before(const Identifier& after, const std::string& clientId) {
    if (after.elements_.empty()) {
        // If the "after" identifier is empty, just create a new one
        return create(clientId);
    }
    
    // Create a new identifier before the given one
    std::vector<IdentifierElement> elements;
    
    // If the first digit is greater than 1, we can create a digit before it
    if (after.elements_[0].getDigit() > 1) {
        elements.emplace_back(after.elements_[0].getDigit() - 1, clientId);
    } else {
        // Otherwise, we need to create a shorter path
        elements.emplace_back(0, clientId);
    }
    
    return Identifier(elements);
}

Identifier Identifier::after(const Identifier& before, const std::string& clientId) {
    if (before.elements_.empty()) {
        // If the "before" identifier is empty, just create a new one
        return create(clientId);
    }
    
    // Create a new identifier after the given one
    std::vector<IdentifierElement> elements;
    
    // If the last digit is less than the max value, we can create a digit after it
    if (before.elements_.back().getDigit() < MAX_DIGIT_VALUE) {
        elements = before.elements_;
        elements.back() = IdentifierElement(before.elements_.back().getDigit() + 1, clientId);
    } else {
        // Otherwise, we need to add a new element
        elements = before.elements_;
        elements.emplace_back(1, clientId);
    }
    
    return Identifier(elements);
}

Identifier Identifier::between(
    const Identifier& before,
    const Identifier& after,
    const std::string& clientId) {
    
    // Handle empty cases
    if (before.elements_.empty()) {
        return before(after, clientId);
    }
    if (after.elements_.empty()) {
        return after(before, clientId);
    }
    
    // Find the first position where the paths differ
    size_t diffPos = 0;
    size_t minSize = std::min(before.elements_.size(), after.elements_.size());
    
    while (diffPos < minSize && 
           before.elements_[diffPos] == after.elements_[diffPos]) {
        diffPos++;
    }
    
    // Case 1: Paths differ at some position
    if (diffPos < minSize) {
        std::vector<IdentifierElement> elements(before.elements_.begin(), 
                                                before.elements_.begin() + diffPos);
        
        uint32_t leftDigit = before.elements_[diffPos].getDigit();
        uint32_t rightDigit = after.elements_[diffPos].getDigit();
        
        // Generate a digit between the two differing digits
        uint32_t newDigit = generateDigitsBetween(leftDigit, rightDigit);
        elements.emplace_back(newDigit, clientId);
        
        return Identifier(elements);
    }
    
    // Case 2: One path is a prefix of the other
    if (before.elements_.size() < after.elements_.size()) {
        // "before" is a prefix of "after"
        std::vector<IdentifierElement> elements = before.elements_;
        elements.emplace_back(after.elements_[diffPos].getDigit() / 2, clientId);
        return Identifier(elements);
    } else {
        // "after" is a prefix of "before"
        std::vector<IdentifierElement> elements(before.elements_.begin(), 
                                                before.elements_.begin() + diffPos);
        elements.emplace_back(before.elements_[diffPos].getDigit() + 1, clientId);
        return Identifier(elements);
    }
}

uint32_t Identifier::generateDigitsBetween(uint32_t left, uint32_t right) {
    // Ensure left < right
    if (left >= right) {
        if (right > 0) {
            return right - 1;
        } else {
            return 0;
        }
    }
    
    // Check if there's space between the digits
    if (right - left <= 1) {
        return left; // No space between, return the left value
    }
    
    // Generate a random digit between left and right
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist(left + 1, right - 1);
    
    return dist(gen);
}

} // namespace ai_editor