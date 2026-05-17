#pragma once
#include "../core/GameState.hpp"
#include "TextureManager.hpp"

#include <imgui.h>
#include <string>

namespace XiangQi {

// -----------------------------------------------------------------------
//  BoardRenderer  – renders the Xiangqi board inside an ImGui window
//                   and translates ImGui mouse events into GameState calls.
//
//  Usage (per frame):
//    renderer.render(gameState);
//
//  The renderer is stateless between frames except for animation timing.
// -----------------------------------------------------------------------
class BoardRenderer {
public:
  // Visual configuration (can be tweaked at runtime)
  struct Config {
    float  boardSize     = 560.0f; // pixel size of the board square region
    float  pieceScale    = 0.85f;  // fraction of cell size for piece sprite
    ImVec4 colorLight    = {0.93f, 0.82f, 0.62f, 1.0f}; // warm wood
    ImVec4 colorDark     = {0.72f, 0.53f, 0.30f, 1.0f}; // darker wood
    ImVec4 colorSelected = {1.0f, 0.85f, 0.0f, 0.7f};   // gold highlight
    ImVec4 colorLegal    = {0.2f, 0.8f, 0.3f, 0.55f};   // green dot
    ImVec4 colorLastMove = {0.3f, 0.6f, 1.0f, 0.45f};   // blue tint
    ImVec4 colorCheck    = {1.0f, 0.2f, 0.1f, 0.55f};   // red tint
    ImVec4 colorGridLine = {0.35f, 0.2f, 0.05f, 1.0f};
    float  gridThickness = 1.5f;
    float  pieceCircleBorderThickness = 2.5f;
    bool   showCoordinates            = true;
    bool   flipBoard                  = false; // Red at top if true
  };

  explicit BoardRenderer(const TextureManager &texMgr);

  // Main render call – draws the board + side panel inside the current
  // ImGui window. Call this inside Begin()/End().
  void render(GameState &gameState);

  Config &config() { return config_; }

  // ------------------------------------------------------------------
  //  PNG layout constants – public so panels can compute aspect ratio
  // ------------------------------------------------------------------
  static constexpr float PNG_W = 900.0f;
  static constexpr float PNG_H = 1000.0f;

private:
  const TextureManager &texMgr_;
  Config                config_;

  // ------------------------------------------------------------------
  //  PNG layout constants (matches xiangqi-gui / xiangqiboard_.png)
  //
  //  The PNG is 900×1000 px.  The actual playfield grid lives inside a
  //  50 px margin on every side:
  //
  //    Grid area : 800 × 900 px  (top-left at (50,50) in PNG space)
  //    Columns   : 9  intersections → 8 intervals → cellW = 800/8 = 100
  //    Rows      : 10 intersections → 9 intervals → cellH = 900/9 = 100
  //
  //  When we render the PNG at size (boardW × boardH) we scale everything
  //  proportionally:
  //    scaleX = boardW / 900
  //    scaleY = boardH / 1000
  //    margin_px_x = 50 * scaleX
  //    margin_px_y = 50 * scaleY
  //    cellW_px    = 100 * scaleX
  //    cellH_px    = 100 * scaleY
  // ------------------------------------------------------------------
  static constexpr float PNG_MARGIN_X = 50.0f; // left & right margin in PNG
  static constexpr float PNG_MARGIN_Y = 50.0f; // top  & bottom margin in PNG
  static constexpr float PNG_GRID_W =
      800.0f; // grid width  in PNG (8 intervals)
  static constexpr float PNG_GRID_H =
      900.0f;                             // grid height in PNG (9 intervals)
  static constexpr int GRID_COLS_INT = 8; // number of column intervals
  static constexpr int GRID_ROWS_INT = 9; // number of row    intervals

  // Board render size: keep aspect ratio 900:1000
  // boardWidth() is set from Config::boardSize (= height)
  float boardWidth() const { return config_.boardSize * (PNG_W / PNG_H); }
  float boardHeight() const { return config_.boardSize; }

  // Scale factors from PNG space to pixel space
  float scaleX() const { return boardWidth() / PNG_W; }
  float scaleY() const { return boardHeight() / PNG_H; }

  // Cell size in pixel space (non-square when window is resized)
  float cellW() const { return (PNG_GRID_W / GRID_COLS_INT) * scaleX(); }
  float cellH() const { return (PNG_GRID_H / GRID_ROWS_INT) * scaleY(); }

  // Top-left pixel of the grid (intersection 0,0 = Black's back rank, col a)
  ImVec2 gridOrigin(ImVec2 boardOrigin) const {
    return {boardOrigin.x + PNG_MARGIN_X * scaleX(),
            boardOrigin.y + PNG_MARGIN_Y * scaleY()};
  }

  // Convert board (row,col) → pixel center of intersection
  ImVec2 squareCenter(Square sq, ImVec2 boardOrigin) const;

  // Convert pixel position → board square (invalid if outside grid)
  Square pixelToSquare(ImVec2 pixel, ImVec2 boardOrigin) const;

  // Drawing primitives  (boardOrigin = top-left of the PNG rect)
  void drawBoard(ImDrawList *dl, ImVec2 boardOrigin) const;
  void
  drawHighlights(ImDrawList *dl, ImVec2 boardOrigin, const GameState &gs) const;
  void drawPieces(ImDrawList *dl, ImVec2 boardOrigin, const Board &board) const;
  void drawOnePiece(ImDrawList *dl, ImVec2 center, const Piece &piece) const;
  void drawCoordinates(ImDrawList *dl, ImVec2 boardOrigin) const;

  // Fallback: draw piece as colored circle with text label
  void drawPieceFallback(ImDrawList  *dl,
                         ImVec2       center,
                         float        radius,
                         const Piece &piece) const;
};

} // namespace XiangQi
