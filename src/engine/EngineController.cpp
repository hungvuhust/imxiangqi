#include "EngineController.hpp"

#include "../core/GameState.hpp"
#include <algorithm>
#include <sstream>

namespace XiangQi {

void EngineController::configure(const EngineSettings &settings) {
  settings_ = settings;
}

bool EngineController::start() {
  stop();

  if (settings_.path.empty()) {
    setError("engine path empty");
    return false;
  }

  state_            = EngineState::Starting;
  detectedProtocol_ = EngineProtocol::Auto;
  sentUcci_         = false;
  sentFallbackUci_  = false;
  waitingReady_     = false;
  activeRequest_    = EngineRequestKind::None;
  pendingHint_.reset();
  pendingMoveUcci_.reset();
  lastInfo_.reset();
  engineOptions_.clear();
  engineIdName_.clear();
  analyzeSnapshot_ = AnalyzeSnapshot{};

  if (!process_.start(settings_.path)) {
    setError("start failed: " + process_.lastError());
    return false;
  }

  log_.push(EngineLogDir::Sys, "engine started: " + settings_.path);
  state_ = EngineState::Handshaking;

  sentUcci_ = true;
  if (!sendLine("ucci")) {
    setError("failed to send ucci");
    return false;
  }

  return true;
}

void EngineController::stop() {
  process_.stop();
  state_           = EngineState::Stopped;
  activeRequest_   = EngineRequestKind::None;
  waitingReady_    = false;
  sentUcci_        = false;
  sentFallbackUci_ = false;
  ponderMove_.clear();
}

bool EngineController::restart() {
  stop();
  return start();
}

void EngineController::update(const GameState & /*game*/) {
  auto lines = process_.pollLines();
  for (auto &l : lines) {
    switch (l.source) {
    case EngineProcess::RawLine::Source::StdOut:
      log_.push(EngineLogDir::Out, l.text);
      handleEngineLine(l.text);
      break;
    case EngineProcess::RawLine::Source::StdErr:
      log_.push(EngineLogDir::Err, l.text);
      break;
    case EngineProcess::RawLine::Source::Sys:
      log_.push(EngineLogDir::Sys, l.text);
      break;
    }
  }

  if ((state_ == EngineState::Handshaking || state_ == EngineState::Ready ||
       state_ == EngineState::Thinking) &&
      !process_.isRunning()) {
    setError("engine process exited");
  }
}

// ---------------------------------------------------------------------------
// Basic search
// ---------------------------------------------------------------------------

bool EngineController::requestMove(const GameState &game) {
  return beginSearch(game, EngineRequestKind::Move);
}

bool EngineController::requestHint(const GameState &game) {
  return beginSearch(game, EngineRequestKind::Hint);
}

void EngineController::cancelThinking() {
  if (state_ != EngineState::Thinking)
    return;
  sendLine("stop");
  // state stays Thinking until bestmove arrives; that's fine
}

// ---------------------------------------------------------------------------
// Analyze mode
// ---------------------------------------------------------------------------

bool EngineController::startAnalyze(const GameState &game) {
  if (state_ != EngineState::Ready)
    return false;

  // Reset snapshot, record whose turn it is for colour-coding arrows
  analyzeSnapshot_           = AnalyzeSnapshot{};
  analyzeSnapshot_.redToMove = (game.sideToMove() == PieceColor::Red);

  std::string fen = game.toFen();
  if (!sendLine("position fen " + fen))
    return false;

  // Send MultiPV setoption before go infinite
  if (settings_.multiPv > 1)
    sendLine("setoption name MultiPV value " +
             std::to_string(settings_.multiPv));
  else
    sendLine("setoption name MultiPV value 1");

  if (!sendLine("go infinite"))
    return false;

  activeRequest_ = EngineRequestKind::Analyze;
  lastInfo_.reset();
  state_ = EngineState::Thinking;
  return true;
}

void EngineController::stopAnalyze() {
  if (activeRequest_ != EngineRequestKind::Analyze)
    return;
  sendLine("stop");
  // state_ will move to Ready when bestmove arrives
}

// ---------------------------------------------------------------------------
// Ponder
// ---------------------------------------------------------------------------

bool EngineController::startPonder(const GameState   &game,
                                   const std::string &ponderMoveUcci) {
  if (state_ != EngineState::Ready || ponderMoveUcci.empty())
    return false;
  if (!settings_.ponder)
    return false;

  ponderMove_ = ponderMoveUcci;

  // Send position + ponder move appended
  std::string fen = game.toFen();
  if (!sendLine("position fen " + fen + " moves " + ponderMoveUcci))
    return false;

  if (!sendLine("go ponder movetime " + std::to_string(settings_.timeMs)))
    return false;

  activeRequest_ = EngineRequestKind::Ponder;
  lastInfo_.reset();
  state_ = EngineState::Thinking;
  return true;
}

void EngineController::stopPonder() {
  if (activeRequest_ != EngineRequestKind::Ponder)
    return;
  sendLine("stop");
}

bool EngineController::sendPonderHit() {
  if (activeRequest_ != EngineRequestKind::Ponder)
    return false;
  // Switch from ponder to real search with ponderhit
  activeRequest_ = EngineRequestKind::Move;
  return sendLine("ponderhit");
}

// ---------------------------------------------------------------------------
// Engine options
// ---------------------------------------------------------------------------

void EngineController::flushDirtyOptions() {
  for (auto &opt : engineOptions_) {
    if (opt.dirty) {
      sendOption(opt);
      opt.dirty = false;
    }
  }
}

void EngineController::sendOption(EngineOption &opt) {
  std::string cmd = EngineProtocolParser::buildSetOption(opt);
  sendLine(cmd);
  opt.dirty = false;
}

// ---------------------------------------------------------------------------
// Consume pending outputs
// ---------------------------------------------------------------------------

bool EngineController::consumePendingMoveUcci(std::string &outMove) {
  if (!pendingMoveUcci_)
    return false;
  outMove = *pendingMoveUcci_;
  pendingMoveUcci_.reset();
  return true;
}

bool EngineController::consumePendingHint(EngineSuggestion &outHint) {
  if (!pendingHint_)
    return false;
  outHint = *pendingHint_;
  pendingHint_.reset();
  return true;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

bool EngineController::sendLine(const std::string &line) {
  log_.push(EngineLogDir::In, line);
  if (!process_.writeLine(line)) {
    setError("write failed: " + process_.lastError());
    return false;
  }
  return true;
}

void EngineController::setError(std::string msg) {
  lastError_ = std::move(msg);
  state_     = EngineState::Error;
  log_.push(EngineLogDir::Sys, "error: " + lastError_);
}

bool EngineController::beginSearch(const GameState  &game,
                                   EngineRequestKind kind,
                                   const std::string & /*ponderMove*/) {
  if (state_ != EngineState::Ready)
    return false;

  std::string fen = game.toFen();
  if (!sendLine("position fen " + fen))
    return false;

  std::ostringstream go;
  if (settings_.timeMs > 0)
    go << "go movetime " << settings_.timeMs;
  else
    go << "go depth " << settings_.depth;

  if (!sendLine(go.str()))
    return false;

  activeRequest_ = kind;
  lastInfo_.reset();
  state_ = EngineState::Thinking;
  return true;
}

void EngineController::updateAnalyzeSnapshot(const EngineInfo &info) {
  int idx = info.multipv - 1;
  if (idx < 0)
    idx = 0;

  // Grow vector if needed
  if (idx >= static_cast<int>(analyzeSnapshot_.pvLines.size()))
    analyzeSnapshot_.pvLines.resize(idx + 1);

  PvLine &pv    = analyzeSnapshot_.pvLines[idx];
  pv.multipv    = info.multipv;
  pv.depth      = info.depth;
  pv.hasScoreCp = info.hasScoreCp;
  pv.scoreCp    = info.scoreCp;
  pv.hasMate    = info.hasMate;
  pv.mate       = info.mate;
  pv.pv         = info.pv;

  if (info.depth > analyzeSnapshot_.depth)
    analyzeSnapshot_.depth = info.depth;
}

void EngineController::handleEngineLine(const std::string &line) {
  // --- Handshaking ---
  if (state_ == EngineState::Handshaking) {
    // Collect id name
    std::string name = EngineProtocolParser::parseIdName(line);
    if (!name.empty()) {
      engineIdName_ = name;
      return;
    }

    // Collect options advertised during handshake
    if (auto opt = EngineProtocolParser::parseOption(line)) {
      // Skip internal-only options we manage ourselves
      if (opt->name != "UCI_Chess960" && opt->name != "UCI_Variant") {
        // Check if already exists (re-handshake scenario)
        auto it = std::find_if(
            engineOptions_.begin(),
            engineOptions_.end(),
            [&](const EngineOption &o) { return o.name == opt->name; });
        if (it == engineOptions_.end())
          engineOptions_.push_back(*opt);
      }
      return;
    }

    if (EngineProtocolParser::isUcciOk(line)) {
      detectedProtocol_ = EngineProtocol::UCCI;
      applyConfiguredOptions();
      sendLine("isready");
      waitingReady_ = true;
      return;
    }

    if (EngineProtocolParser::isUciOk(line)) {
      detectedProtocol_ = EngineProtocol::UCI;
      applyConfiguredOptions();
      sendLine("isready");
      waitingReady_ = true;
      return;
    }

    if (!sentFallbackUci_ && sentUcci_ &&
        line.find("Unknown command") != std::string::npos) {
      sentFallbackUci_ = true;
      sendLine("uci");
      return;
    }
  }

  // --- Waiting for readyok ---
  if (waitingReady_ && EngineProtocolParser::isReadyOk(line)) {
    waitingReady_ = false;
    state_        = EngineState::Ready;
    log_.push(EngineLogDir::Sys, "engine ready");
    return;
  }

  // --- info lines (during Thinking) ---
  EngineInfo info;
  if (lastInfo_)
    info = *lastInfo_;
  if (EngineProtocolParser::parseInfo(line, info)) {
    lastInfo_ = info;
    // Live update analyze snapshot when in analyze mode
    if (activeRequest_ == EngineRequestKind::Analyze)
      updateAnalyzeSnapshot(info);
    return;
  }

  // --- bestmove ---
  auto best =
      EngineProtocolParser::parseBestMove(line,
                                          lastInfo_.value_or(EngineInfo{}));
  if (!best)
    return;

  if (activeRequest_ == EngineRequestKind::Move) {
    pendingMoveUcci_ = best->moveUcci;
  } else if (activeRequest_ == EngineRequestKind::Hint) {
    pendingHint_ = *best;
  } else if (activeRequest_ == EngineRequestKind::Analyze) {
    // Analyze mode received bestmove (after stop); just go back to Ready
    // analyzeSnapshot_ already has the last info
  } else if (activeRequest_ == EngineRequestKind::Ponder) {
    // Ponder was stopped without ponderhit — discard
  }

  activeRequest_ = EngineRequestKind::None;
  if (!waitingReady_)
    state_ = EngineState::Ready;
}

void EngineController::applyConfiguredOptions() {
  sendLine("setoption name Ponder value " +
           std::string(settings_.ponder ? "true" : "false"));
  sendLine("setoption name MultiPV value " + std::to_string(settings_.multiPv));
}

} // namespace XiangQi
