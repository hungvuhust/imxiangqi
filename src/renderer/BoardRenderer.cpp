#include "BoardRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>
#include <optional>
#include <string>

namespace XiangQi {

// =======================================================================
//  Internal colour helpers
// =======================================================================
static ImU32 toU32(ImVec4 v) {
  return IM_COL32(static_cast<int>(v.x * 255),
                  static_cast<int>(v.y * 255),
                  static_cast<int>(v.z * 255),
                  static_cast<int>(v.w * 255));
}

// =======================================================================
//  Construction
// =======================================================================
BoardRenderer::BoardRenderer(const TextureManager &texMgr)
    : texMgr_(texMgr) {}

// =======================================================================
//  Coordinate mapping
//
//  The PNG (900×1000) has a 50 px margin on every side.
//  Intersection (row, col) sits at PNG coords:
//    png_x = 50 + col * (800/8)  = 50 + col * 100
//    png_y = 50 + row * (900/9)  = 50 + row * 100
//
//  Scaled to the rendered board rect (boardWidth × boardHeight):
//    pixel_x = boardOrigin.x + png_x * scaleX
//    pixel_y = boardOrigin.y + png_y * scaleY
// =======================================================================
ImVec2 BoardRenderer::squareCenter(Square sq, ImVec2 boardOrigin) const {
  int displayRow = config_.flipBoard ? (Board::ROWS - 1 - sq.row) : sq.row;
  int displayCol = config_.flipBoard ? (Board::COLS - 1 - sq.col) : sq.col;

  float png_x = PNG_MARGIN_X + displayCol * (PNG_GRID_W / GRID_COLS_INT);
  float png_y = PNG_MARGIN_Y + displayRow * (PNG_GRID_H / GRID_ROWS_INT);

  return {boardOrigin.x + png_x * scaleX(), boardOrigin.y + png_y * scaleY()};
}

Square BoardRenderer::pixelToSquare(ImVec2 pixel, ImVec2 boardOrigin) const {
  // Inverse: pixel → PNG space → grid coords
  float png_x = (pixel.x - boardOrigin.x) / scaleX();
  float png_y = (pixel.y - boardOrigin.y) / scaleY();

  float grid_x = png_x - PNG_MARGIN_X;
  float grid_y = png_y - PNG_MARGIN_Y;

  float cellW_png = PNG_GRID_W / GRID_COLS_INT; // 100
  float cellH_png = PNG_GRID_H / GRID_ROWS_INT; // 100

  int displayCol = static_cast<int>(std::round(grid_x / cellW_png));
  int displayRow = static_cast<int>(std::round(grid_y / cellH_png));

  int col = config_.flipBoard ? (Board::COLS - 1 - displayCol) : displayCol;
  int row = config_.flipBoard ? (Board::ROWS - 1 - displayRow) : displayRow;

  if (col < 0 || col >= Board::COLS || row < 0 || row >= Board::ROWS)
    return {};
  return {static_cast<int8_t>(row), static_cast<int8_t>(col)};
}

// =======================================================================
//  Board background
// =======================================================================
void BoardRenderer::drawBoard(ImDrawList *dl, ImVec2 boardOrigin) const {
  const Texture *boardTex = texMgr_.boardTexture();
  ImVec2         tl       = boardOrigin;
  ImVec2 br = {boardOrigin.x + boardWidth(), boardOrigin.y + boardHeight()};

  if (boardTex && boardTex->valid()) {
    dl->AddImage(boardTex->imguiID(), tl, br);
  } else {
    // Fallback: plain wood-coloured rect + hand-drawn grid
    dl->AddRectFilled(tl, br, toU32(config_.colorLight));

    ImU32  lineCol = toU32(config_.colorGridLine);
    ImVec2 go      = gridOrigin(boardOrigin);
    float  cw      = cellW();
    float  ch      = cellH();

    // Horizontal lines
    for (int r = 0; r < Board::ROWS; ++r) {
      float y = go.y + r * ch;
      dl->AddLine({go.x, y}, {go.x + cw * GRID_COLS_INT, y}, lineCol, 1.5f);
    }
    // Vertical lines (skip middle segment on inner columns for the river)
    for (int c = 0; c < Board::COLS; ++c) {
      float x = go.x + c * cw;
      if (c == 0 || c == Board::COLS - 1) {
        dl->AddLine({x, go.y}, {x, go.y + ch * GRID_ROWS_INT}, lineCol, 1.5f);
      } else {
        // Top half (rows 0-4)
        dl->AddLine({x, go.y}, {x, go.y + 4 * ch}, lineCol, 1.5f);
        // Bottom half (rows 5-9)
        dl->AddLine({x, go.y + 5 * ch}, {x, go.y + 9 * ch}, lineCol, 1.5f);
      }
    }
    // Palace diagonals – Black (rows 0-2, cols 3-5)
    {
      ImVec2 p0 = {go.x + 3 * cw, go.y + 0 * ch};
      ImVec2 p1 = {go.x + 5 * cw, go.y + 2 * ch};
      ImVec2 p2 = {go.x + 5 * cw, go.y + 0 * ch};
      ImVec2 p3 = {go.x + 3 * cw, go.y + 2 * ch};
      dl->AddLine(p0, p1, lineCol, 1.5f);
      dl->AddLine(p2, p3, lineCol, 1.5f);
    }
    // Palace diagonals – Red (rows 7-9, cols 3-5)
    {
      ImVec2 p0 = {go.x + 3 * cw, go.y + 7 * ch};
      ImVec2 p1 = {go.x + 5 * cw, go.y + 9 * ch};
      ImVec2 p2 = {go.x + 5 * cw, go.y + 7 * ch};
      ImVec2 p3 = {go.x + 3 * cw, go.y + 9 * ch};
      dl->AddLine(p0, p1, lineCol, 1.5f);
      dl->AddLine(p2, p3, lineCol, 1.5f);
    }
  }
}

// =======================================================================
//  Highlights  (last move, check, selection, legal destinations)
// =======================================================================
void BoardRenderer::drawHighlights(ImDrawList      *dl,
                                   ImVec2           boardOrigin,
                                   const GameState &gs) const {
  const Board &board = gs.board();
  float        hw    = cellW() * 0.48f; // half-width of highlight rect
  float        hh    = cellH() * 0.48f;

  auto fillRect = [&](Square sq, ImU32 col) {
    ImVec2 c = squareCenter(sq, boardOrigin);
    dl->AddRectFilled({c.x - hw, c.y - hh}, {c.x + hw, c.y + hh}, col);
  };

  // Last move tint
  if (!board.history().empty()) {
    const Move &last    = board.history().back();
    ImU32       lastCol = toU32(config_.colorLastMove);
    fillRect(last.from(), lastCol);
    fillRect(last.to(), lastCol);
  }

  // King-in-check
  if (board.isInCheck()) {
    Square king = board.findKing(board.sideToMove());
    if (king.valid())
      fillRect(king, toU32(config_.colorCheck));
  }

  // Selected piece
  const Selection &sel = gs.selection();
  if (sel.phase == SelectionPhase::PieceSelected) {
    fillRect(sel.square, toU32(config_.colorSelected));

    // Legal destinations
    for (const auto &m : sel.legalDests) {
      ImVec2 dc = squareCenter(m.to(), boardOrigin);
      if (board.at(m.to()).empty()) {
        // Empty square: small circle at intersection
        float r = std::min(cellW(), cellH()) * 0.18f;
        dl->AddCircleFilled(dc, r, toU32(config_.colorLegal));
      } else {
        // Capture: ring
        float r = std::min(cellW(), cellH()) * 0.44f;
        dl->AddCircle(dc, r, toU32(config_.colorLegal), 24, 3.0f);
      }
    }
  }
}

// =======================================================================
//  Piece rendering
// =======================================================================
void BoardRenderer::drawPieceFallback(ImDrawList  *dl,
                                      ImVec2       center,
                                      float        radius,
                                      const Piece &piece) const {
  ImU32 bgColor = (piece.color() == PieceColor::Red)
                      ? IM_COL32(220, 50, 30, 255)
                      : IM_COL32(20, 20, 20, 255);
  ImU32 fgColor = IM_COL32(255, 220, 150, 255);
  ImU32 rimCol  = IM_COL32(200, 170, 100, 255);

  dl->AddCircleFilled(center, radius, bgColor);
  dl->AddCircle(center, radius, rimCol, 32, config_.pieceCircleBorderThickness);

  // Chinese character label
  const char *label = "?";
  if (piece.color() == PieceColor::Red) {
    switch (piece.type()) {
    case PieceType::King: label = "\xe5\xb8\xa5"; break;     // 帥
    case PieceType::Advisor: label = "\xe4\xbb\x95"; break;  // 仕
    case PieceType::Elephant: label = "\xe7\x9b\xb8"; break; // 相
    case PieceType::Knight: label = "\xe9\xa6\xac"; break;   // 馬
    case PieceType::Rook: label = "\xe8\xbb\x8a"; break;     // 車
    case PieceType::Cannon: label = "\xe7\x82\xae"; break;   // 炮
    case PieceType::Pawn: label = "\xe5\x85\xb5"; break;     // 兵
    default: break;
    }
  } else {
    switch (piece.type()) {
    case PieceType::King: label = "\xe5\xb0\x87"; break;     // 將
    case PieceType::Advisor: label = "\xe5\xa3\xab"; break;  // 士
    case PieceType::Elephant: label = "\xe8\xb1\xa1"; break; // 象
    case PieceType::Knight: label = "\xe9\xa6\xac"; break;   // 馬
    case PieceType::Rook: label = "\xe8\xbb\x8a"; break;     // 車
    case PieceType::Cannon: label = "\xe7\xa0\xb2"; break;   // 砲
    case PieceType::Pawn: label = "\xe5\x8d\x92"; break;     // 卒
    default: break;
    }
  }

  ImVec2 ts   = ImGui::CalcTextSize(label);
  ImVec2 tPos = {center.x - ts.x * 0.5f, center.y - ts.y * 0.5f};
  dl->AddText(tPos, fgColor, label);
}

void BoardRenderer::drawOnePiece(ImDrawList  *dl,
                                 ImVec2       center,
                                 const Piece &piece) const {
  // Piece sprite is scaled so it fits inside the cell with pieceScale margin.
  // Use the smaller of cellW/cellH to keep it circular.
  float halfSz       = std::min(cellW(), cellH()) * config_.pieceScale * 0.5f;
  const Texture *tex = texMgr_.pieceTexture(piece);

  if (tex && tex->valid()) {
    ImVec2 tl = {center.x - halfSz, center.y - halfSz};
    ImVec2 br = {center.x + halfSz, center.y + halfSz};
    dl->AddImage(tex->imguiID(), tl, br);
  } else {
    drawPieceFallback(dl, center, halfSz, piece);
  }
}

void BoardRenderer::drawPieces(ImDrawList  *dl,
                               ImVec2       boardOrigin,
                               const Board &board) const {
  for (int r = 0; r < Board::ROWS; ++r)
    for (int c = 0; c < Board::COLS; ++c) {
      const Piece &p = board.at(r, c);
      if (p.empty())
        continue;
      Square sq{static_cast<int8_t>(r), static_cast<int8_t>(c)};
      ImVec2 center = squareCenter(sq, boardOrigin);
      drawOnePiece(dl, center, p);
    }
}

// =======================================================================
//  Coordinates
// =======================================================================
void BoardRenderer::drawCoordinates(ImDrawList *dl, ImVec2 boardOrigin) const {
  if (!config_.showCoordinates)
    return;

  ImU32 textCol  = IM_COL32(60, 40, 10, 200);
  float fontSize = std::min(cellW(), cellH()) * 0.28f;
  if (fontSize < 9.0f)
    fontSize = 9.0f;

  ImVec2 go = gridOrigin(boardOrigin);

  // Column labels  a-i  (below the grid)
  static constexpr const char *colLabels[9] =
      {"a", "b", "c", "d", "e", "f", "g", "h", "i"};
  for (int c = 0; c < Board::COLS; ++c) {
    int   dc = config_.flipBoard ? (Board::COLS - 1 - c) : c;
    float x  = go.x + c * cellW() - fontSize * 0.3f;
    float y  = go.y + GRID_ROWS_INT * cellH() + 3.0f;
    dl->AddText(nullptr, fontSize, {x, y}, textCol, colLabels[dc]);
  }

  // Row labels  0-9  (left of the grid)
  for (int r = 0; r < Board::ROWS; ++r) {
    int  dr   = config_.flipBoard ? (Board::ROWS - 1 - r) : r;
    int  rank = 9 - dr; // row 0 (Black back rank) = rank 9
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", rank);
    float y = go.y + r * cellH() - fontSize * 0.5f;
    float x = go.x - cellW() * 0.6f;
    dl->AddText(nullptr, fontSize, {x, y}, textCol, buf);
  }
}

void BoardRenderer::drawHintArrow(
    ImDrawList                       *dl,
    ImVec2                            boardOrigin,
    const std::optional<std::string> &hintMoveUcci) const {
  if (!hintMoveUcci.has_value() || hintMoveUcci->size() < 4)
    return;

  Square from = Square::fromString(hintMoveUcci->substr(0, 2));
  Square to   = Square::fromString(hintMoveUcci->substr(2, 2));
  if (!from.valid() || !to.valid())
    return;

  ImVec2 p0 = squareCenter(from, boardOrigin);
  ImVec2 p1 = squareCenter(to, boardOrigin);

  ImVec2 d{p1.x - p0.x, p1.y - p0.y};
  float  len = std::sqrt(d.x * d.x + d.y * d.y);
  if (len < 1.0f)
    return;

  ImVec2 n{d.x / len, d.y / len};
  ImVec2 perp{-n.y, n.x};

  float minCell   = std::min(cellW(), cellH());
  float headLen   = std::max(12.0f, minCell * 0.34f);
  float headWidth = headLen * 0.60f;
  float thick     = std::max(2.0f, minCell * 0.08f);

  ImVec2 tip  = p1;
  ImVec2 base = {tip.x - n.x * headLen, tip.y - n.y * headLen};
  ImVec2 l    = {base.x + perp.x * headWidth * 0.5f,
                 base.y + perp.y * headWidth * 0.5f};
  ImVec2 r    = {base.x - perp.x * headWidth * 0.5f,
                 base.y - perp.y * headWidth * 0.5f};

  ImVec2 shaftEnd = {base.x - n.x * 2.0f, base.y - n.y * 2.0f};

  ImU32 lineCol = IM_COL32(255, 190, 40, 235);
  ImU32 tipCol  = IM_COL32(255, 145, 0, 245);

  dl->AddLine(p0, shaftEnd, lineCol, thick);
  dl->AddTriangleFilled(tip, l, r, tipCol);
  dl->AddCircleFilled(p0, std::max(2.0f, thick * 0.8f), lineCol);
}

// =======================================================================
//  PV arrows  (Analyze mode MultiPV visualisation)
// =======================================================================

// ---------------------------------------------------------------------------
// Draw a filled convex 5-pointed star centred at (cx,cy), outer radius r.
// ---------------------------------------------------------------------------
static void drawStar(ImDrawList *dl,
                     float       cx,
                     float       cy,
                     float       r,
                     ImU32       col,
                     float       innerRatio = 0.42f) {
  static constexpr int N = 5;
  ImVec2               pts[10];
  for (int i = 0; i < N; ++i) {
    float ao = -static_cast<float>(M_PI) / 2.0f +
               i * 2.0f * static_cast<float>(M_PI) / N;
    float ai       = ao + static_cast<float>(M_PI) / N;
    pts[2 * i]     = {cx + r * std::cos(ao), cy + r * std::sin(ao)};
    pts[2 * i + 1] = {cx + r * innerRatio * std::cos(ai),
                      cy + r * innerRatio * std::sin(ai)};
  }
  dl->AddConvexPolyFilled(pts, 10, col);
}

// ---------------------------------------------------------------------------
// Draw a star outline (no fill).
// ---------------------------------------------------------------------------
static void drawStarOutline(ImDrawList *dl,
                            float       cx,
                            float       cy,
                            float       r,
                            ImU32       col,
                            float       thickness  = 1.5f,
                            float       innerRatio = 0.42f) {
  static constexpr int N = 5;
  ImVec2               pts[11]; // closed polyline
  for (int i = 0; i < N; ++i) {
    float ao = -static_cast<float>(M_PI) / 2.0f +
               i * 2.0f * static_cast<float>(M_PI) / N;
    float ai       = ao + static_cast<float>(M_PI) / N;
    pts[2 * i]     = {cx + r * std::cos(ao), cy + r * std::sin(ao)};
    pts[2 * i + 1] = {cx + r * innerRatio * std::cos(ai),
                      cy + r * innerRatio * std::sin(ai)};
  }
  pts[10] = pts[0]; // close
  dl->AddPolyline(pts, 11, col, ImDrawFlags_None, thickness);
}

void BoardRenderer::drawPvArrows(ImDrawList            *dl,
                                 ImVec2                 boardOrigin,
                                 const AnalyzeSnapshot &snap) const {
  if (snap.pvLines.empty())
    return;

  // ------------------------------------------------------------------
  // Base colour by side-to-move  (pastel)
  // Red side  → pastel red  R:255 G:140 B:140
  // Black side → pastel blue R:110 G:170 B:255
  // ------------------------------------------------------------------
  const ImVec4 baseF = snap.redToMove ? ImVec4(1.0f, 0.55f, 0.55f, 1.0f)
                                      : ImVec4(0.43f, 0.67f, 1.0f, 1.0f);

  // Non-best arrows use the same hue but more transparent
  auto makeCol = [&](float alpha) -> ImU32 {
    return IM_COL32(static_cast<int>(baseF.x * 255),
                    static_cast<int>(baseF.y * 255),
                    static_cast<int>(baseF.z * 255),
                    static_cast<int>(alpha * 255));
  };

  // ------------------------------------------------------------------
  // Find best PV index (highest score from engine's POV)
  // ------------------------------------------------------------------
  auto scoreOf = [](const PvLine &pv) -> int {
    if (pv.hasMate)   return 100000 - std::abs(pv.mate) * 10;
    if (pv.hasScoreCp) return pv.scoreCp;
    return -999999;
  };
  int bestIdx = 0;
  for (int i = 1; i < static_cast<int>(snap.pvLines.size()); ++i)
    if (scoreOf(snap.pvLines[i]) > scoreOf(snap.pvLines[bestIdx]))
      bestIdx = i;

  // ------------------------------------------------------------------
  // Sizing
  // ------------------------------------------------------------------
  float minCell = std::min(cellW(), cellH());
  float thick   = std::max(6.0f, minCell * 0.13f);  // thicker shaft
  float headLen = std::max(18.0f, minCell * 0.38f);
  float headW   = headLen * 0.70f;
  float fontSize = std::max(11.0f, minCell * 0.20f);

  // ------------------------------------------------------------------
  // Draw each PV line
  // ------------------------------------------------------------------
  for (int idx = 0; idx < static_cast<int>(snap.pvLines.size()); ++idx) {
    const PvLine &pv      = snap.pvLines[idx];
    const bool    isBest  = (idx == bestIdx);

    if (pv.pv.empty()) continue;

    // First move token, e.g. "h9g7"
    std::string firstMove = pv.pv.substr(0, pv.pv.find(' '));
    if (firstMove.size() < 4) continue;

    Square from = Square::fromString(firstMove.substr(0, 2));
    Square to   = Square::fromString(firstMove.substr(2, 2));
    if (!from.valid() || !to.valid()) continue;

    // Exact grid-intersection centres (no extra offset)
    ImVec2 p0 = squareCenter(from, boardOrigin);
    ImVec2 p1 = squareCenter(to,   boardOrigin);

    ImVec2 d   = {p1.x - p0.x, p1.y - p0.y};
    float  len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 1.0f) continue;

    ImVec2 n    = {d.x / len, d.y / len};      // unit direction
    ImVec2 perp = {-n.y, n.x};                 // perpendicular

    // Slight lateral offset so multiple arrows don't perfectly stack
    float  laneOffset =
        (snap.pvLines.size() > 1)
            ? (idx - static_cast<float>(snap.pvLines.size() - 1) * 0.5f) *
                  (thick * 1.6f)
            : 0.0f;
    ImVec2 shift = {perp.x * laneOffset, perp.y * laneOffset};

    // Shaft: starts exactly at intersection p0, ends at arrowhead base
    ImVec2 A        = {p0.x + shift.x, p0.y + shift.y};
    ImVec2 B        = {p1.x + shift.x, p1.y + shift.y};
    ImVec2 tip      = B;
    ImVec2 baseHead = {tip.x - n.x * headLen, tip.y - n.y * headLen};
    ImVec2 shaftEnd = {baseHead.x - n.x * 1.5f, baseHead.y - n.y * 1.5f};

    // Arrowhead wing points
    ImVec2 lwing = {baseHead.x + perp.x * headW * 0.5f,
                    baseHead.y + perp.y * headW * 0.5f};
    ImVec2 rwing = {baseHead.x - perp.x * headW * 0.5f,
                    baseHead.y - perp.y * headW * 0.5f};

    // Alpha: best PV full opacity, others semi-transparent
    float  alpha    = isBest ? 0.90f : 0.55f;
    ImU32  col      = makeCol(alpha);
    ImU32  colDim   = makeCol(alpha * 0.35f); // dark outline tint

    // ---- Shaft outline (dark halo) ----
    dl->AddLine(A, shaftEnd, IM_COL32(0, 0, 0, static_cast<int>(alpha * 110)),
                thick + 3.0f);
    // ---- Shaft fill ----
    dl->AddLine(A, shaftEnd, col, thick);

    // ---- Arrowhead outline ----
    dl->AddTriangleFilled(tip, lwing, rwing,
                          IM_COL32(0, 0, 0, static_cast<int>(alpha * 110)));
    // ---- Arrowhead fill ----
    ImVec2 tipIn  = {tip.x  + n.x * 1.5f, tip.y  + n.y * 1.5f};
    ImVec2 lwingIn = {lwing.x - n.x * 0.8f, lwing.y - n.y * 0.8f};
    ImVec2 rwingIn = {rwing.x - n.x * 0.8f, rwing.y - n.y * 0.8f};
    dl->AddTriangleFilled(tipIn, lwingIn, rwingIn, col);

    // ---- Origin dot at start intersection ----
    float dotR = thick * 0.65f;
    dl->AddCircleFilled(A, dotR + 1.5f,
                        IM_COL32(0, 0, 0, static_cast<int>(alpha * 110)));
    dl->AddCircleFilled(A, dotR, col);

    // ------------------------------------------------------------------
    // Score badge — star-shaped background
    // ------------------------------------------------------------------
    // Build label: float with 2 decimals, e.g. "+1.50"
    char label[32];
    if (pv.hasMate)
      snprintf(label, sizeof(label), "M%d", pv.mate);
    else if (pv.hasScoreCp)
      snprintf(label, sizeof(label), "%+.2f", pv.scoreCp / 100.0f);
    else
      snprintf(label, sizeof(label), "?");

    // Badge sits at shaft midpoint
    ImVec2 mid = {(A.x + shaftEnd.x) * 0.5f, (A.y + shaftEnd.y) * 0.5f};

    // Measure text at target font size
    float  scale     = fontSize / ImGui::GetFontSize();
    ImVec2 rawSize   = ImGui::CalcTextSize(label);
    float  textW     = rawSize.x * scale;
    float  textH     = rawSize.y * scale;

    // Star outer radius sized to contain the text comfortably
    float  starR     = std::max(textW, textH) * 0.62f + 6.0f;

    ImU32  bgDark    = IM_COL32(15, 15, 15, static_cast<int>(alpha * 210));
    ImU32  colBright = makeCol(std::min(1.0f, alpha + 0.1f));

    if (isBest) {
      // Best PV: filled star badge
      drawStar(dl, mid.x, mid.y, starR, bgDark);       // dark fill
      drawStar(dl, mid.x, mid.y, starR, colDim);        // tinted fill overlay
      drawStarOutline(dl, mid.x, mid.y, starR,
                      colBright, 2.0f);                 // bright border
    } else {
      // Non-best: star outline only
      drawStar(dl, mid.x, mid.y, starR, bgDark);        // dark fill
      drawStarOutline(dl, mid.x, mid.y, starR,
                      col, 1.5f);                        // outline
    }

    // Score text centred inside star
    ImVec2 textPos = {mid.x - textW * 0.5f, mid.y - textH * 0.5f};
    dl->AddText(nullptr, fontSize, textPos, colBright, label);
  }
}

// =======================================================================
//  Main render entry point
//  Called by BoardPanel::onRender() – no side-panel logic here.
// =======================================================================
void BoardRenderer::render(GameState                        &gameState,
                           const std::optional<std::string> &hintMoveUcci,
                           const AnalyzeSnapshot            &analyzeSnapshot) {
  ImVec2 boardOrigin = ImGui::GetCursorScreenPos();

  // Invisible button covers the entire board PNG area for mouse capture
  ImGui::InvisibleButton("##board", {boardWidth(), boardHeight()});
  bool boardHovered = ImGui::IsItemHovered();

  ImDrawList *dl = ImGui::GetWindowDrawList();

  // 1. Board PNG (or fallback grid)
  drawBoard(dl, boardOrigin);

  // 2. Highlights (under pieces)
  drawHighlights(dl, boardOrigin, gameState);

  // 3. Coordinates
  drawCoordinates(dl, boardOrigin);

  // 4. PV arrows from analyze mode (drawn before hint, under pieces)
  drawPvArrows(dl, boardOrigin, analyzeSnapshot);

  // 5. Hint arrow
  drawHintArrow(dl, boardOrigin, hintMoveUcci);

  // 6. Pieces (on top of arrows)
  drawPieces(dl, boardOrigin, gameState.board());

  // 7. Mouse click → GameState
  if (boardHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
    Square sq = pixelToSquare(ImGui::GetMousePos(), boardOrigin);
    if (sq.valid())
      gameState.onSquareClicked(sq);
  }
}

} // namespace XiangQi
