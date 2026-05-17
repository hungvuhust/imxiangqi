#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace XiangQi {

enum class PlayerMode : uint8_t {
  Human,  // user clicks on the board
  Engine, // UCI/UCCI engine process
};

enum class EngineProtocol : uint8_t {
  Auto,
  UCCI,
  UCI,
};

enum class EngineState : uint8_t {
  Stopped,
  Starting,
  Handshaking,
  Ready,
  Thinking,
  Error,
};

enum class EngineRequestKind : uint8_t {
  None,
  Move,
  Hint,
};

enum class EngineLogDir : uint8_t {
  In,
  Out,
  Err,
  Sys,
};

struct EngineInfo {
  int         depth      = -1;
  bool        hasScoreCp = false;
  int         scoreCp    = 0;
  bool        hasMate    = false;
  int         mate       = 0;
  std::string pv;
};

struct EngineSuggestion {
  std::string moveUcci;
  EngineInfo  info;
};

struct EngineLogEntry {
  uint64_t                              seq = 0;
  std::chrono::system_clock::time_point ts{};
  EngineLogDir                          dir  = EngineLogDir::Sys;
  std::string                           text = {};
};

inline const char *toString(EngineProtocol p) {
  switch (p) {
  case EngineProtocol::Auto: return "Auto";
  case EngineProtocol::UCCI: return "UCCI";
  case EngineProtocol::UCI: return "UCI";
  }
  return "Unknown";
}

inline const char *toString(EngineState s) {
  switch (s) {
  case EngineState::Stopped: return "Stopped";
  case EngineState::Starting: return "Starting";
  case EngineState::Handshaking: return "Handshaking";
  case EngineState::Ready: return "Ready";
  case EngineState::Thinking: return "Thinking";
  case EngineState::Error: return "Error";
  }
  return "Unknown";
}

inline const char *toString(EngineLogDir d) {
  switch (d) {
  case EngineLogDir::In: return "IN";
  case EngineLogDir::Out: return "OUT";
  case EngineLogDir::Err: return "ERR";
  case EngineLogDir::Sys: return "SYS";
  }
  return "?";
}

} // namespace XiangQi
