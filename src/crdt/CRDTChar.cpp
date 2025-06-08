#include "crdt/CRDTChar.hpp"
#include <nlohmann/json.hpp>

namespace ai_editor {

using json = nlohmann::json;

CRDTChar::CRDTChar(
    char value,
    const Identifier& position,
    const std::string& clientId,
    uint64_t clock,
    bool deleted)
    : value_(value),
      position_(position),
      clientId_(clientId),
      clock_(clock),
      deleted_(deleted) {}

char CRDTChar::getValue() const {
    return value_;
}

const Identifier& CRDTChar::getPosition() const {
    return position_;
}

const std::string& CRDTChar::getClientId() const {
    return clientId_;
}

uint64_t CRDTChar::getClock() const {
    return clock_;
}

bool CRDTChar::isDeleted() const {
    return deleted_;
}

void CRDTChar::markDeleted(bool deleted) {
    deleted_ = deleted;
}

std::string CRDTChar::toJson() const {
    json j;
    
    j["value"] = std::string(1, value_);
    j["position"] = json::parse(position_.toJson());
    j["clientId"] = clientId_;
    j["clock"] = clock_;
    j["deleted"] = deleted_;
    
    return j.dump();
}

std::shared_ptr<CRDTChar> CRDTChar::fromJson(const std::string& jsonStr) {
    json j = json::parse(jsonStr);
    
    char value = j["value"].get<std::string>()[0];
    Identifier position = Identifier::fromJson(j["position"].dump());
    std::string clientId = j["clientId"].get<std::string>();
    uint64_t clock = j["clock"].get<uint64_t>();
    bool deleted = j["deleted"].get<bool>();
    
    return std::make_shared<CRDTChar>(value, position, clientId, clock, deleted);
}

bool CRDTChar::operator==(const CRDTChar& other) const {
    return value_ == other.value_ &&
           position_ == other.position_ &&
           clientId_ == other.clientId_ &&
           clock_ == other.clock_ &&
           deleted_ == other.deleted_;
}

bool CRDTChar::operator<(const CRDTChar& other) const {
    return position_ < other.position_;
}

} // namespace ai_editor 