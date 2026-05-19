#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace XiangQi {

enum class PlayerMode : uint8_t {
  Human,  // user clicks on the board
  Engine, // UCI/UCCI engine process
};

// -----------------------------------------------------------------------
//  EngineOption  – one option advertised by the engine via "option name ..."
// -----------------------------------------------------------------------
enum class EngineOptionType : uint8_t {
  Check,  // boolean
  Spin,   // integer with min/max
  Combo,  // string enum
  Button, // stateless trigger
  String, // free text
};

struct EngineOption {
  std::string      name;
  EngineOptionType type = EngineOptionType::String;

  // Spin
  int spinVal = 0;
  int spinMin = 0;
  int spinMax = 0;

  // Check
  bool checkVal = false;

  // Combo / String
  std::string              strVal;
  std::vector<std::string> comboVars; // valid values for combo

  // Dirty flag: set when user changes value so we send setoption
  bool dirty = false;
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
  Analyze, // continuous analysis; no bestmove expected
  Ponder,  // thinking on opponent's time
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
  int         multipv    = 1; // which PV line (1-based)
  std::string pv;
};

struct EngineSuggestion {
  std::string moveUcci;
  EngineInfo  info;
};

// One PV line in a MultiPV analysis
struct PvLine {
  int         multipv    = 1;
  int         depth      = -1;
  bool        hasScoreCp = false;
  int         scoreCp    = 0;
  bool        hasMate    = false;
  int         mate       = 0;
  std::string pv;
};

// Live analysis snapshot (updated every info line)
struct AnalyzeSnapshot {
  std::vector<PvLine> pvLines;        // indexed by multipv-1
  int                 depth    = -1;  // max depth seen
  bool                redToMove = true; // whose turn it is when analysis started
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
