#pragma once
#include "../core/GameSettings.hpp"
#include "../core/GameState.hpp"
#include "../core/Piece.hpp"
#include "../engine/EnginePool.hpp"
#include "../renderer/TextureManager.hpp"
#include <imgui.h>
#include <optional>

namespace XiangQi {

// -----------------------------------------------------------------------
//  AppContext  – single shared "hub" passed by reference to every panel.
// -----------------------------------------------------------------------
struct AppContext {
  GameState            &gameState;
  GameSettings         &settings;
  const TextureManager &texMgr;
  ImFont               *fontVN = nullptr;

  std::optional<EngineSuggestion> hint;

  // Convenience: engine assigned to the current side-to-move (may be null)
  EngineController *activeEngine() {
    return settings.pool.activeEngineFor(gameState.sideToMove() ==
                                         PieceColor::Red);
  }

  // Convenience forwarders
  void newGame() { gameState.newGame(); }
  void undoMove() {
    if (gameState.canUndo())
      gameState.undoMove();
  }
  void loadFen(const std::string &fen) { gameState.loadFen(fen); }
};

} // namespace XiangQi
