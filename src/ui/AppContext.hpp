#pragma once
#include "../core/GameSettings.hpp"
#include "../core/GameState.hpp"
#include "../engine/EngineController.hpp"
#include "../renderer/TextureManager.hpp"
#include <imgui.h>
#include <optional>

namespace XiangQi {

// -----------------------------------------------------------------------
//  AppContext  – single shared "hub" passed by reference to every panel.
//
//  No panel owns any of these objects; they all live in Application.
//  Panels may READ everything and WRITE through the defined mutators of
//  GameState / GameSettings  (no raw pointer hacks).
//
//  This replaces the old pattern where BoardRenderer called
//  gameState.newGame() / undoMove() directly – now those calls go through
//  AppContext so panels stay decoupled from each other.
// -----------------------------------------------------------------------
struct AppContext {
  GameState            &gameState;
  GameSettings         &settings;
  EngineController     &engineRed;   // engine cho bên Đỏ
  EngineController     &engineBlack; // engine cho bên Đen
  const TextureManager &texMgr;
  ImFont               *fontVN = nullptr; // Roboto with Vietnamese glyphs

  std::optional<EngineSuggestion> hint;

  // Trả về engine của bên đang đến lượt
  EngineController &activeEngine() {
    return (gameState.sideToMove() == PieceColor::Red) ? engineRed
                                                       : engineBlack;
  }

  // Convenience forwarders so panels don't need to know GameState internals
  void newGame() { gameState.newGame(); }
  void undoMove() {
    if (gameState.canUndo())
      gameState.undoMove();
  }
  void loadFen(const std::string &fen) { gameState.loadFen(fen); }
};

} // namespace XiangQi
