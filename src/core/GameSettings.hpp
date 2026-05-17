#pragma once
#include <string>

namespace XiangQi {

// -----------------------------------------------------------------------
//  PlayerMode  – who controls each side
// -----------------------------------------------------------------------
enum class PlayerMode {
  Human,  // user clicks on the board
  Engine, // UCI/UCCI engine process (future)
};

// -----------------------------------------------------------------------
//  BoardTheme  – visual preset for the board
// -----------------------------------------------------------------------
enum class BoardTheme {
  Classic, // original PNG wood texture
  Simple,  // flat vector fallback
};

// -----------------------------------------------------------------------
//  GameSettings  – serialisable plain-data struct
//
//  Owns all user-configurable state that survives a "New Game":
//    - who plays Red / Black
//    - engine command & depth
//    - board visual options
//    - coordinate display
//
//  Non-persistent runtime state (selection, move history, etc.)
//  lives in GameState, not here.
// -----------------------------------------------------------------------
struct GameSettings {
  // ------------------------------------------------------------------
  //  Player configuration
  // ------------------------------------------------------------------
  PlayerMode redPlayer   = PlayerMode::Human;
  PlayerMode blackPlayer = PlayerMode::Human;

  // ------------------------------------------------------------------
  //  Engine configuration  (used when PlayerMode == Engine)
  // ------------------------------------------------------------------
  std::string enginePath   = "";    // path to engine binary
  std::string engineName   = "";    // display name
  int         engineDepth  = 15;    // search depth
  int         engineTimeMs = 3000;  // time per move (ms)
  bool        enginePonder = false; // pondering on opponent's turn

  // ------------------------------------------------------------------
  //  Board visual
  // ------------------------------------------------------------------
  BoardTheme theme        = BoardTheme::Classic;
  float      pieceScale   = 0.85f; // fraction of cell occupied by piece
  bool       flipBoard    = false; // Black at bottom if true
  bool       showCoords   = true;  // show a-i / 0-9 labels
  bool       animateMoves = true;  // piece slide animation (future)
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
  //  Helpers
  // ------------------------------------------------------------------
  bool isTwoPlayer() const {
    return redPlayer == PlayerMode::Human && blackPlayer == PlayerMode::Human;
  }
  bool hasEngine() const {
    return redPlayer == PlayerMode::Engine || blackPlayer == PlayerMode::Engine;
  }

  // Simple serialisation to / from INI-style string (for future save/load)
  std::string         serialize() const;
  static GameSettings deserialize(const std::string &data);

  // Reset to factory defaults
  void reset() { *this = GameSettings{}; }
};

} // namespace XiangQi
