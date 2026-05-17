#pragma once

#include "EngineLog.hpp"
#include "EngineProcess.hpp"
#include "EngineProtocol.hpp"
#include "EngineSettings.hpp"

#include <optional>
#include <string>

namespace XiangQi {

class GameState;

class EngineController {
public:
  EngineController() = default;

  void configure(const EngineSettings &settings);

  bool start();
  void stop();
  bool restart();

  void update(const GameState &game);

  bool requestMove(const GameState &game);
  bool requestHint(const GameState &game);
  void cancelThinking();

  bool consumePendingMoveUcci(std::string &outMove);
  bool consumePendingHint(EngineSuggestion &outHint);

  bool isRunning() const { return process_.isRunning(); }
  bool isReady() const { return state_ == EngineState::Ready; }
  bool isThinking() const { return state_ == EngineState::Thinking; }

  EngineState    state() const { return state_; }
  EngineProtocol protocol() const { return detectedProtocol_; }

  const std::string               &lastError() const { return lastError_; }
  const std::optional<EngineInfo> &lastInfo() const { return lastInfo_; }

  EngineLog       &log() { return log_; }
  const EngineLog &log() const { return log_; }

private:
  bool sendLine(const std::string &line);
  void setError(std::string msg);
  bool beginSearch(const GameState &game, EngineRequestKind kind);

  void handleEngineLine(const std::string &line);
  void applyConfiguredOptions();

  EngineSettings settings_{};

  EngineProcess process_;
  EngineLog     log_;

  EngineState    state_            = EngineState::Stopped;
  EngineProtocol detectedProtocol_ = EngineProtocol::Auto;

  bool sentUcci_        = false;
  bool sentFallbackUci_ = false;
  bool waitingReady_    = false;

  EngineRequestKind activeRequest_ = EngineRequestKind::None;

  std::optional<EngineInfo>       lastInfo_;
  std::optional<EngineSuggestion> pendingHint_;
  std::optional<std::string>      pendingMoveUcci_;

  std::string lastError_;
};

} // namespace XiangQi
