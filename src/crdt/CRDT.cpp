#include "crdt/CRDT.hpp"
#include <nlohmann/json.hpp>

namespace ai_editor {

using json = nlohmann::json;

CRDT::CRDT(
    const std::string& clientId,
    std::shared_ptr<ICRDTStrategy> strategy)
    : clientId_(clientId),
      strategy_(strategy) {
    initializeStrategyIfNeeded();
}

std::shared_ptr<CRDTChar> CRDT::localInsert(char c, size_t index) {
    // Generate the next clock value for this client
    auto yataStrategy = std::dynamic_pointer_cast<YataStrategy>(strategy_);
    uint64_t clock = yataStrategy->getNextClientClock(clientId_);
    
    // Insert the character using the strategy
    return strategy_->insert(c, index, clientId_, clock);
}

bool CRDT::localDelete(size_t index) {
    // Generate the next clock value for this client
    auto yataStrategy = std::dynamic_pointer_cast<YataStrategy>(strategy_);
    uint64_t clock = yataStrategy->getNextClientClock(clientId_);
    
    // Delete the character using the strategy
    return strategy_->remove(index, clientId_, clock);
}

bool CRDT::remoteInsert(const std::shared_ptr<CRDTChar>& character) {
    // Apply the remote insert using the strategy
    return strategy_->applyRemoteInsert(character);
}

bool CRDT::remoteDelete(
    const Identifier& position,
    const std::string& clientId,
    uint64_t clock) {
    // Apply the remote delete using the strategy
    return strategy_->applyRemoteDelete(position, clientId, clock);
}

std::string CRDT::toString() const {
    return strategy_->toString();
}

std::string CRDT::getClientId() const {
    return clientId_;
}

std::shared_ptr<ICRDTStrategy> CRDT::getStrategy() const {
    return strategy_;
}

void CRDT::setStrategy(std::shared_ptr<ICRDTStrategy> strategy) {
    strategy_ = strategy;
    initializeStrategyIfNeeded();
}

std::string CRDT::toJson() const {
    json j;
    
    j["clientId"] = clientId_;
    j["strategy"] = strategy_->getStrategyName();
    j["content"] = json::parse(strategy_->toJson());
    
    return j.dump();
}

std::shared_ptr<CRDT> CRDT::fromJson(
    const std::string& jsonStr,
    const std::string& clientId) {
    json j = json::parse(jsonStr);
    
    // Create a new CRDT with the given client ID
    auto crdt = std::make_shared<CRDT>(clientId);
    
    // Parse the strategy type and content
    std::string strategyName = j["strategy"].get<std::string>();
    std::string strategyContent = j["content"].dump();
    
    // Create the appropriate strategy
    if (strategyName == "YATA") {
        crdt->setStrategy(YataStrategy::fromJson(strategyContent, clientId));
    } else {
        // Default to YATA strategy if unknown
        crdt->setStrategy(std::make_shared<YataStrategy>(clientId));
    }
    
    return crdt;
}

void CRDT::initializeStrategyIfNeeded() {
    if (!strategy_) {
        strategy_ = std::make_shared<YataStrategy>(clientId_);
    }
}

// Static method implementation for ICRDT
std::shared_ptr<ICRDT> ICRDT::fromJson(
    const std::string& json,
    const std::string& clientId) {
    return CRDT::fromJson(json, clientId);
}

} // namespace ai_editor 