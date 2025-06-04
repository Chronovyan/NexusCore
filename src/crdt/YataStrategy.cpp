#include "crdt/YataStrategy.hpp"
#include "crdt/CRDTChar.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace ai_editor {

using json = nlohmann::json;

YataStrategy::YataStrategy(const std::string& clientId)
    : clientId_(clientId) {
    vectorClock_[clientId_] = 0;
}

std::shared_ptr<CRDTChar> YataStrategy::insert(
    char value,
    size_t index,
    const std::string& clientId,
    uint64_t clock) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update the vector clock
    updateVectorClock(clientId, clock);
    
    // Generate a position for the new character
    Identifier position = generatePositionBetween(index);
    
    // Create a new CRDT character
    auto character = std::make_shared<CRDTChar>(
        value,
        position,
        clientId,
        clock,
        false);
    
    // Insert the character
    if (index >= chars_.size()) {
        chars_.push_back(character);
    } else {
        chars_.insert(chars_.begin() + index, character);
    }
    
    return character;
}

bool YataStrategy::remove(
    size_t index,
    const std::string& clientId,
    uint64_t clock) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update the vector clock
    updateVectorClock(clientId, clock);
    
    // Find the character to delete
    if (index >= size()) {
        return false;
    }
    
    auto character = getCharAt(index);
    if (!character) {
        return false;
    }
    
    // Mark the character as deleted
    character->markDeleted(true);
    
    return true;
}

std::shared_ptr<CRDTChar> YataStrategy::at(size_t index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getCharAt(index);
}

size_t YataStrategy::size(bool includeDeleted) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (includeDeleted) {
        return chars_.size();
    }
    
    // Count only non-deleted characters
    return std::count_if(chars_.begin(), chars_.end(),
        [](const std::shared_ptr<CRDTChar>& c) { return !c->isDeleted(); });
}

std::string YataStrategy::toString() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::stringstream ss;
    for (const auto& c : chars_) {
        if (!c->isDeleted()) {
            ss << c->getValue();
        }
    }
    
    return ss.str();
}

std::optional<size_t> YataStrategy::findByPosition(const Identifier& position) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (size_t i = 0; i < chars_.size(); ++i) {
        if (chars_[i]->getPosition() == position) {
            return i;
        }
    }
    
    return std::nullopt;
}

bool YataStrategy::applyRemoteInsert(const std::shared_ptr<CRDTChar>& character) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update the vector clock
    updateVectorClock(character->getClientId(), character->getClock());
    
    // Find the insertion index
    size_t index = findInsertIndex(character);
    
    // Insert the character
    if (index >= chars_.size()) {
        chars_.push_back(character);
    } else {
        chars_.insert(chars_.begin() + index, character);
    }
    
    return true;
}

bool YataStrategy::applyRemoteDelete(
    const Identifier& position,
    const std::string& clientId,
    uint64_t clock) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update the vector clock
    updateVectorClock(clientId, clock);
    
    // Find the character to delete
    auto index = findByPosition(position);
    if (!index) {
        return false;
    }
    
    // Mark the character as deleted
    chars_[*index]->markDeleted(true);
    
    return true;
}

std::string YataStrategy::getStrategyName() const {
    return "YATA";
}

std::vector<std::shared_ptr<CRDTChar>> YataStrategy::getAllChars(bool includeDeleted) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (includeDeleted) {
        return chars_;
    }
    
    // Filter out deleted characters
    std::vector<std::shared_ptr<CRDTChar>> result;
    std::copy_if(chars_.begin(), chars_.end(), std::back_inserter(result),
        [](const std::shared_ptr<CRDTChar>& c) { return !c->isDeleted(); });
    
    return result;
}

uint64_t YataStrategy::getClientClock(const std::string& clientId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = vectorClock_.find(clientId);
    if (it != vectorClock_.end()) {
        return it->second;
    }
    
    return 0;
}

uint64_t YataStrategy::getNextClientClock(const std::string& clientId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& clock = vectorClock_[clientId];
    return ++clock;
}

std::unordered_map<std::string, uint64_t> YataStrategy::getVectorClock() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return vectorClock_;
}

std::string YataStrategy::toJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json j;
    
    // Serialize characters
    json chars = json::array();
    for (const auto& c : chars_) {
        chars.push_back({
            {"value", c->getValue()},
            {"position", c->getPosition().toJson()},
            {"clientId", c->getClientId()},
            {"clock", c->getClock()},
            {"deleted", c->isDeleted()}
        });
    }
    
    j["chars"] = chars;
    
    // Serialize vector clock
    json vclock = json::object();
    for (const auto& [client, clock] : vectorClock_) {
        vclock[client] = clock;
    }
    
    j["vectorClock"] = vclock;
    
    return j.dump();
}

std::shared_ptr<YataStrategy> YataStrategy::fromJson(
    const std::string& jsonStr,
    const std::string& clientId) {
    json j = json::parse(jsonStr);
    
    auto strategy = std::make_shared<YataStrategy>(clientId);
    
    // Deserialize characters
    if (j.contains("chars") && j["chars"].is_array()) {
        for (const auto& charJson : j["chars"]) {
            char value = charJson["value"].get<char>();
            Identifier position = Identifier::fromJson(charJson["position"].dump());
            std::string charClientId = charJson["clientId"].get<std::string>();
            uint64_t clock = charJson["clock"].get<uint64_t>();
            bool deleted = charJson["deleted"].get<bool>();
            
            auto character = std::make_shared<CRDTChar>(
                value,
                position,
                charClientId,
                clock,
                deleted);
            
            strategy->chars_.push_back(character);
        }
    }
    
    // Deserialize vector clock
    if (j.contains("vectorClock") && j["vectorClock"].is_object()) {
        for (auto it = j["vectorClock"].begin(); it != j["vectorClock"].end(); ++it) {
            strategy->vectorClock_[it.key()] = it.value().get<uint64_t>();
        }
    }
    
    // Ensure the client ID is in the vector clock
    if (strategy->vectorClock_.find(clientId) == strategy->vectorClock_.end()) {
        strategy->vectorClock_[clientId] = 0;
    }
    
    return strategy;
}

size_t YataStrategy::findInsertIndex(const std::shared_ptr<CRDTChar>& character) const {
    // Find the first position that is greater than the character's position
    auto it = std::lower_bound(chars_.begin(), chars_.end(), character,
        [](const std::shared_ptr<CRDTChar>& a, const std::shared_ptr<CRDTChar>& b) {
            return a->getPosition() < b->getPosition();
        });
    
    return std::distance(chars_.begin(), it);
}

Identifier YataStrategy::generatePositionBetween(size_t index) const {
    // Get the characters before and after the insertion point
    std::shared_ptr<CRDTChar> before;
    std::shared_ptr<CRDTChar> after;
    
    if (index > 0 && index <= chars_.size()) {
        before = getCharAt(index - 1, true);
    }
    
    if (index < chars_.size()) {
        after = getCharAt(index, true);
    }
    
    // Generate a position between the two characters
    if (before && after) {
        return Identifier::between(before->getPosition(), after->getPosition(), clientId_);
    } else if (before) {
        return Identifier::after(before->getPosition(), clientId_);
    } else if (after) {
        return Identifier::before(after->getPosition(), clientId_);
    } else {
        // No characters yet, create a new position
        return Identifier::create(clientId_);
    }
}

std::shared_ptr<CRDTChar> YataStrategy::getCharAt(size_t index, bool includeDeleted) const {
    if (includeDeleted) {
        if (index >= chars_.size()) {
            return nullptr;
        }
        return chars_[index];
    }
    
    // Count only non-deleted characters
    size_t visibleIndex = 0;
    for (const auto& c : chars_) {
        if (!c->isDeleted()) {
            if (visibleIndex == index) {
                return c;
            }
            visibleIndex++;
        }
    }
    
    return nullptr;
}

void YataStrategy::updateVectorClock(const std::string& clientId, uint64_t clock) {
    auto& currentClock = vectorClock_[clientId];
    currentClock = std::max(currentClock, clock);
}

} // namespace ai_editor 