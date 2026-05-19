#pragma once
#include "../engine/EnginePool.hpp"
#include <string>

namespace XiangQi {

// -----------------------------------------------------------------------
//  BoardTheme  – visual preset for the board
// -----------------------------------------------------------------------
enum class BoardTheme {
  Classic, // original PNG wood texture
  Simple,  // flat vector fallback
};

// -----------------------------------------------------------------------
//  GameSettings  – serialisable config that survives a "New Game"
//
//  Player assignment lives in pool.redIdx / pool.blackIdx:
//    -1  → Human
//    >=0 → index into pool.entries[]
// -----------------------------------------------------------------------
struct GameSettings {
  // ------------------------------------------------------------------
  //  Engine pool  (owns N EngineController instances)
  // ------------------------------------------------------------------
  EnginePool pool;

  // ------------------------------------------------------------------
  //  Board visual
  // ------------------------------------------------------------------
  BoardTheme theme        = BoardTheme::Classic;
  float      pieceScale   = 0.85f;
  bool       flipBoard    = false;
  bool       showCoords   = true;
  bool       animateMoves = true;
  float      animSpeedMs  = 120.0f;

  // ------------------------------------------------------------------
  //  Highlight colours (RGBA 0-1)
  // ------------------------------------------------------------------
  struct Color4 {
    float r, g, b, a;
  };
  Color4 colSelected = {1.0f, 0.85f, 0.0f, 0.70f};
  Color4 colLegal    = {0.2f, 0.8f, 0.3f, 0.55f};
  Color4 colLastMove = {0.3f, 0.6f, 1.0f, 0.45f};
  Color4 colCheck    = {1.0f, 0.2f, 0.1f, 0.55f};

  // ------------------------------------------------------------------
  //  Helpers (delegate to pool)
  // ------------------------------------------------------------------
  bool hasEngine() const { return pool.hasAnyEngine(); }

  // Serialisation
  std::string         serialize() const;
  static GameSettings deserialize(const std::string &data);

  // Reset to factory defaults (keeps pool entries but stops all engines)
  void reset() {
    pool.stopAll();
    pool.entries.clear();
    pool.redIdx   = -1;
    pool.blackIdx = -1;
    theme         = BoardTheme::Classic;
    pieceScale    = 0.85f;
    flipBoard     = false;
    showCoords    = true;
    animateMoves  = true;
    animSpeedMs   = 120.0f;
    colSelected   = {1.0f, 0.85f, 0.0f, 0.70f};
    colLegal      = {0.2f, 0.8f, 0.3f, 0.55f};
    colLastMove   = {0.3f, 0.6f, 1.0f, 0.45f};
    colCheck      = {1.0f, 0.2f, 0.1f, 0.55f};
  }
};

} // namespace XiangQi
