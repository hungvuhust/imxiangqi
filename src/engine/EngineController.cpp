#include "EngineController.hpp"

#include "../core/GameState.hpp"
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
}

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
                                   EngineRequestKind kind) {
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

void EngineController::handleEngineLine(const std::string &line) {
  if (state_ == EngineState::Handshaking) {
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

  if (waitingReady_ && EngineProtocolParser::isReadyOk(line)) {
    waitingReady_ = false;
    state_        = EngineState::Ready;
    log_.push(EngineLogDir::Sys, "engine ready");
    return;
  }

  EngineInfo info;
  if (lastInfo_)
    info = *lastInfo_;
  if (EngineProtocolParser::parseInfo(line, info)) {
    lastInfo_ = info;
    return;
  }

  auto best =
      EngineProtocolParser::parseBestMove(line,
                                          lastInfo_.value_or(EngineInfo{}));
  if (!best)
    return;

  if (activeRequest_ == EngineRequestKind::Move) {
    pendingMoveUcci_ = best->moveUcci;
  } else if (activeRequest_ == EngineRequestKind::Hint) {
    pendingHint_ = *best;
  }

  activeRequest_ = EngineRequestKind::None;
  if (!waitingReady_)
    state_ = EngineState::Ready;
}

void EngineController::applyConfiguredOptions() {
  if (settings_.ponder)
    sendLine("setoption name Ponder value true");
  else
    sendLine("setoption name Ponder value false");

  if (settings_.depth > 0) {
    sendLine("setoption name MultiPV value 1");
  }
}

} // namespace XiangQi
