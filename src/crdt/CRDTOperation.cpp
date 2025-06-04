#include "crdt/CRDTOperation.hpp"
#include "interfaces/ICRDT.hpp"
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace ai_editor {

// CRDTOperation implementation
CRDTOperation::CRDTOperation(
    CRDTOperationType type,
    const std::string& clientId,
    uint64_t clock)
    : type_(type),
      clientId_(clientId),
      clock_(clock) {}

CRDTOperationType CRDTOperation::getType() const {
    return type_;
}

std::string CRDTOperation::getClientId() const {
    return clientId_;
}

uint64_t CRDTOperation::getClock() const {
    return clock_;
}

// CRDTInsertOperation implementation
CRDTInsertOperation::CRDTInsertOperation(
    char value,
    const Identifier& position,
    const std::string& clientId,
    uint64_t clock)
    : CRDTOperation(CRDTOperationType::INSERT, clientId, clock),
      value_(value),
      position_(position) {}

bool CRDTInsertOperation::apply(ICRDTStrategy& strategy) {
    try {
        // Create a new CRDTChar and apply it to the strategy
        auto character = std::make_shared<CRDTChar>(
            value_, position_, clientId_, clock_, false);
        
        return strategy.applyRemoteInsert(character);
    } catch (const std::exception& e) {
        // Handle exceptions
        return false;
    }
}

std::string CRDTInsertOperation::toJson() const {
    nlohmann::json jsonObj;
    
    jsonObj["type"] = "insert";
    jsonObj["value"] = std::string(1, value_);
    jsonObj["position"] = nlohmann::json::parse(position_.toJson());
    jsonObj["clientId"] = clientId_;
    jsonObj["clock"] = clock_;
    
    return jsonObj.dump();
}

std::shared_ptr<ICRDTOperation> CRDTInsertOperation::invert() const {
    // The inverse of an insert is a delete
    return std::make_shared<CRDTDeleteOperation>(position_, clientId_, clock_);
}

char CRDTInsertOperation::getValue() const {
    return value_;
}

const Identifier& CRDTInsertOperation::getPosition() const {
    return position_;
}

std::shared_ptr<CRDTInsertOperation> CRDTInsertOperation::fromJson(const std::string& json) {
    try {
        auto jsonObj = nlohmann::json::parse(json);
        
        char value = jsonObj["value"].get<std::string>()[0];
        Identifier position = Identifier::fromJson(jsonObj["position"].dump());
        std::string clientId = jsonObj["clientId"];
        uint64_t clock = jsonObj["clock"];
        
        return std::make_shared<CRDTInsertOperation>(value, position, clientId, clock);
    } catch (const std::exception& e) {
        // Handle parsing errors
        return nullptr;
    }
}

// CRDTDeleteOperation implementation
CRDTDeleteOperation::CRDTDeleteOperation(
    const Identifier& position,
    const std::string& clientId,
    uint64_t clock)
    : CRDTOperation(CRDTOperationType::DELETE, clientId, clock),
      position_(position) {}

bool CRDTDeleteOperation::apply(ICRDTStrategy& strategy) {
    try {
        return strategy.applyRemoteDelete(position_, clientId_, clock_);
    } catch (const std::exception& e) {
        // Handle exceptions
        return false;
    }
}

std::string CRDTDeleteOperation::toJson() const {
    nlohmann::json jsonObj;
    
    jsonObj["type"] = "delete";
    jsonObj["position"] = nlohmann::json::parse(position_.toJson());
    jsonObj["clientId"] = clientId_;
    jsonObj["clock"] = clock_;
    
    return jsonObj.dump();
}

std::shared_ptr<ICRDTOperation> CRDTDeleteOperation::invert() const {
    // We can't truly invert a delete without knowing the character value
    // In practice, this would require storing the deleted character's value
    // or retrieving it from the document state
    throw std::runtime_error("Cannot invert a delete operation without character value");
}

const Identifier& CRDTDeleteOperation::getPosition() const {
    return position_;
}

std::shared_ptr<CRDTDeleteOperation> CRDTDeleteOperation::fromJson(const std::string& json) {
    try {
        auto jsonObj = nlohmann::json::parse(json);
        
        Identifier position = Identifier::fromJson(jsonObj["position"].dump());
        std::string clientId = jsonObj["clientId"];
        uint64_t clock = jsonObj["clock"];
        
        return std::make_shared<CRDTDeleteOperation>(position, clientId, clock);
    } catch (const std::exception& e) {
        // Handle parsing errors
        return nullptr;
    }
}

// CRDTCompositeOperation implementation
CRDTCompositeOperation::CRDTCompositeOperation(
    const std::vector<std::shared_ptr<ICRDTOperation>>& operations,
    const std::string& clientId,
    uint64_t clock)
    : CRDTOperation(CRDTOperationType::COMPOSITE, clientId, clock),
      operations_(operations) {}

bool CRDTCompositeOperation::apply(ICRDTStrategy& strategy) {
    // Apply all component operations in sequence
    for (const auto& operation : operations_) {
        if (!operation->apply(strategy)) {
            // If any operation fails, the composite operation fails
            return false;
        }
    }
    
    return true;
}

std::string CRDTCompositeOperation::toJson() const {
    nlohmann::json jsonObj;
    
    jsonObj["type"] = "composite";
    jsonObj["clientId"] = clientId_;
    jsonObj["clock"] = clock_;
    
    nlohmann::json operationsArray = nlohmann::json::array();
    for (const auto& operation : operations_) {
        operationsArray.push_back(nlohmann::json::parse(operation->toJson()));
    }
    jsonObj["operations"] = operationsArray;
    
    return jsonObj.dump();
}

std::shared_ptr<ICRDTOperation> CRDTCompositeOperation::invert() const {
    // Invert all component operations in reverse order
    std::vector<std::shared_ptr<ICRDTOperation>> invertedOperations;
    
    for (auto it = operations_.rbegin(); it != operations_.rend(); ++it) {
        try {
            invertedOperations.push_back((*it)->invert());
        } catch (const std::exception& e) {
            // If any operation can't be inverted, the composite can't be inverted
            throw std::runtime_error("Cannot invert a composite operation: " + std::string(e.what()));
        }
    }
    
    return std::make_shared<CRDTCompositeOperation>(invertedOperations, clientId_, clock_);
}

const std::vector<std::shared_ptr<ICRDTOperation>>& CRDTCompositeOperation::getOperations() const {
    return operations_;
}

std::shared_ptr<CRDTCompositeOperation> CRDTCompositeOperation::fromJson(const std::string& json) {
    try {
        auto jsonObj = nlohmann::json::parse(json);
        
        std::string clientId = jsonObj["clientId"];
        uint64_t clock = jsonObj["clock"];
        
        std::vector<std::shared_ptr<ICRDTOperation>> operations;
        for (const auto& opJson : jsonObj["operations"]) {
            std::string opType = opJson["type"];
            
            if (opType == "insert") {
                operations.push_back(CRDTInsertOperation::fromJson(opJson.dump()));
            } else if (opType == "delete") {
                operations.push_back(CRDTDeleteOperation::fromJson(opJson.dump()));
            } else if (opType == "composite") {
                operations.push_back(CRDTCompositeOperation::fromJson(opJson.dump()));
            }
        }
        
        return std::make_shared<CRDTCompositeOperation>(operations, clientId, clock);
    } catch (const std::exception& e) {
        // Handle parsing errors
        return nullptr;
    }
}

// Static method to deserialize an operation from JSON
std::shared_ptr<ICRDTOperation> ICRDTOperation::fromJson(const std::string& json) {
    try {
        auto jsonObj = nlohmann::json::parse(json);
        std::string type = jsonObj["type"];
        
        if (type == "insert") {
            return CRDTInsertOperation::fromJson(json);
        } else if (type == "delete") {
            return CRDTDeleteOperation::fromJson(json);
        } else if (type == "composite") {
            return CRDTCompositeOperation::fromJson(json);
        } else {
            throw std::runtime_error("Unknown operation type: " + type);
        }
    } catch (const std::exception& e) {
        // Handle parsing errors
        return nullptr;
    }
}

} // namespace ai_editor 