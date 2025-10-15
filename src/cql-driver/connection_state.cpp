#include "connection_state.hpp"
#include <userver/utils/trivial_map.hpp>

namespace cql {

namespace {
userver::utils::TrivialBiMap kConnectionStatusToString = [](auto selector) {
  return selector()
      .Case(ConnectionState::Status::NOT_CONNECTED, "NOT_CONNECTED")
      .Case(ConnectionState::Status::CONNECTING, "CONNECTING")
      .Case(ConnectionState::Status::CONNECTED, "CONNECTED")
      .Case(ConnectionState::Status::AUTHENTICATING, "AUTHENTICATING")
      .Case(ConnectionState::Status::CLOSING, "CLOSING")
      .Case(ConnectionState::Status::CLOSED, "CLOSED")
      .Case(ConnectionState::Status::ERROR, "ERROR");
};
}

const std::string ConnectionState::StatusToString(Status status) noexcept {
  auto stringified = kConnectionStatusToString.TryFind(status);
  if (stringified->empty()) {
    return "UNKNOWN";
  } else {
    return stringified.value().data();
  }
}
}  // namespace cql
