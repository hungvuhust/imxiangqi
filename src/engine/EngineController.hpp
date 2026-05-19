#pragma once

#include "EngineLog.hpp"
#include "EngineProcess.hpp"
#include "EngineProtocol.hpp"
#include "EngineSettings.hpp"

#include <optional>
#include <string>
#include <vector>

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

  // --- Basic search ---
  bool requestMove(const GameState &game);
  bool requestHint(const GameState &game);
  void cancelThinking();

  // --- Analyze mode ---
  bool startAnalyze(const GameState &game);
  void stopAnalyze();
  bool isAnalyzing() const {
    return activeRequest_ == EngineRequestKind::Analyze;
  }
  const AnalyzeSnapshot &analyzeSnapshot() const { return analyzeSnapshot_; }

  // --- Ponder ---
  bool startPonder(const GameState &game, const std::string &ponderMoveUcci);
  void stopPonder(); // sends "stop"; caller must call ponderhit or just discard
  bool isPondering() const {
    return activeRequest_ == EngineRequestKind::Ponder;
  }
  // Call when opponent played the expected ponder move
  bool sendPonderHit();

  // --- Engine options ---
  const std::vector<EngineOption> &engineOptions() const {
    return engineOptions_;
  }
  std::vector<EngineOption> &engineOptions() { return engineOptions_; }
  // Send all dirty options to engine; clears dirty flags
  void                       flushDirtyOptions();
  // Send a specific option immediately
  void                       sendOption(EngineOption &opt);
  // Detected engine name (from "id name")
  const std::string         &engineIdName() const { return engineIdName_; }

  // --- Consume outputs ---
  bool consumePendingMoveUcci(std::string &outMove);
  bool consumePendingHint(EngineSuggestion &outHint);

  // --- State queries ---
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
  bool beginSearch(const GameState   &game,
                   EngineRequestKind  kind,
                   const std::string &ponderMove = {});

  void handleEngineLine(const std::string &line);
  void applyConfiguredOptions();
  void updateAnalyzeSnapshot(const EngineInfo &info);

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

  // Analyze mode
  AnalyzeSnapshot analyzeSnapshot_;

  // Ponder: the move we expect the opponent to play
  std::string ponderMove_;

  // Engine options advertised during handshake
  std::vector<EngineOption> engineOptions_;
  std::string               engineIdName_;

  std::string lastError_;
};

} // namespace XiangQi
