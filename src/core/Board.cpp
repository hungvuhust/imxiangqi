#include "Board.hpp"
#include <cassert>
#include <sstream>
#include <stdexcept>

namespace XiangQi {

// =======================================================================
//  Construction
// =======================================================================
Board::Board() { reset(); }
Board::Board(const std::string &fen) { loadFen(fen); }

void Board::reset() { loadFen(INITIAL_FEN); }

// =======================================================================
//  Grid access
// =======================================================================
const Piece &Board::at(int row, int col) const {
  assert(row >= 0 && row < ROWS && col >= 0 && col < COLS);
  return cells_[row][col];
}

Piece &Board::atMut(int row, int col) {
  assert(row >= 0 && row < ROWS && col >= 0 && col < COLS);
  return cells_[row][col];
}

void Board::setAt(int row, int col, const Piece &p) { cells_[row][col] = p; }

bool Board::inBounds(int row, int col) const {
  return row >= 0 && row < ROWS && col >= 0 && col < COLS;
}

// =======================================================================
//  FEN  (UCCI format)
//  Example: "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - -
//  0 1"
// =======================================================================
void Board::loadFen(const std::string &fen) {
  // Clear board
  for (auto &row : cells_)
    for (auto &cell : row) cell = Piece::Empty;
  history_.clear();

  std::istringstream ss(fen);
  std::string        boardPart, sidePart, dummy;
  int                halfMove = 0, fullMove = 1;

  ss >> boardPart >> sidePart >> dummy >> dummy >> halfMove >> fullMove;

  // Parse board part
  int row = 0, col = 0;
  for (char c : boardPart) {
    if (c == '/') {
      ++row;
      col = 0;
    } else if (c >= '1' && c <= '9') {
      col += (c - '0');
    } else {
      if (inBounds(row, col))
        setAt(row, col, Piece::fromFenChar(c));
      ++col;
    }
  }

  side_     = (sidePart == "b") ? PieceColor::Black : PieceColor::Red;
  halfMove_ = halfMove;
  fullMove_ = fullMove;
}

std::string Board::toFen() const {
  std::string fen;

  for (int r = 0; r < ROWS; ++r) {
    if (r > 0)
      fen += '/';
    int empty = 0;
    for (int c = 0; c < COLS; ++c) {
      const Piece &p = at(r, c);
      if (p.empty()) {
        ++empty;
      } else {
        if (empty) {
          fen += static_cast<char>('0' + empty);
          empty = 0;
        }
        fen += p.toFenChar();
      }
    }
    if (empty)
      fen += static_cast<char>('0' + empty);
  }

  fen += (side_ == PieceColor::Black) ? " b" : " w";
  fen += " - - ";
  fen += std::to_string(halfMove_);
  fen += ' ';
  fen += std::to_string(fullMove_);
  return fen;
}

// =======================================================================
//  Palace helper
// =======================================================================
bool Board::inPalace(int r, int c, PieceColor color) const {
  if (c < 3 || c > 5)
    return false;
  if (color == PieceColor::Red)
    return r >= 7 && r <= 9;
  if (color == PieceColor::Black)
    return r >= 0 && r <= 2;
  return false;
}

// =======================================================================
//  Flying generals
// =======================================================================
bool Board::flyingGenerals() const {
  Square redKing   = findKing(PieceColor::Red);
  Square blackKing = findKing(PieceColor::Black);
  if (!redKing.valid() || !blackKing.valid())
    return false;
  if (redKing.col != blackKing.col)
    return false;

  int minRow = std::min(redKing.row, blackKing.row) + 1;
  int maxRow = std::max(redKing.row, blackKing.row);
  for (int r = minRow; r < maxRow; ++r)
    if (!isEmpty(r, redKing.col))
      return false;
  return true; // kings face each other with nothing between them
}

// =======================================================================
//  Find king
// =======================================================================
Square Board::findKing(PieceColor side) const {
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c)
      if (at(r, c).type() == PieceType::King && at(r, c).color() == side)
        return {static_cast<int8_t>(r), static_cast<int8_t>(c)};
  return {};
}

// =======================================================================
//  isAttacked  – Is square (r,c) threatened by any piece of byColor?
// =======================================================================
bool Board::isAttacked(int r, int c, PieceColor byColor) const {
  // Temporarily place a dummy piece so cannon/rook scans work correctly
  // We just scan from every piece of byColor to see if it can reach (r,c).
  // Easier: generate all pseudo-legal moves for byColor and check targets.
  for (int pr = 0; pr < ROWS; ++pr)
    for (int pc = 0; pc < COLS; ++pc) {
      const Piece &p = at(pr, pc);
      if (p.empty() || p.color() != byColor)
        continue;
      auto moves = pseudoMovesFor(pr, pc);
      for (const auto &m : moves)
        if (m.to().row == r && m.to().col == c)
          return true;
    }
  return false;
}

// =======================================================================
//  isInCheck
// =======================================================================
bool Board::isInCheck(PieceColor side) const {
  Square king = findKing(side);
  if (!king.valid())
    return false;
  return isAttacked(king.row, king.col, opposite(side));
}

bool Board::isInCheck() const { return isInCheck(side_); }

// =======================================================================
//  Move legality filter
// =======================================================================
bool Board::leavesKingInCheck(const Move &m) const {
  // Make a temporary copy, apply move, test
  Board tmp = *this;
  tmp.doMove(m);
  // The moving side is side_ (before the move flipped it inside doMove),
  // but doMove flips side_, so we test the OTHER current side (which is
  // the side that just moved).
  PieceColor movingSide = m.moved().color();
  return tmp.isInCheck(movingSide) || tmp.flyingGenerals();
}

// =======================================================================
//  Pseudo-legal move dispatcher
// =======================================================================
std::vector<Move> Board::pseudoMovesFor(int row, int col) const {
  const Piece &p = at(row, col);
  if (p.empty())
    return {};

  std::vector<Move> out;
  out.reserve(16);

  switch (p.type()) {
  case PieceType::King: genKing(row, col, p.color(), out); break;
  case PieceType::Advisor: genAdvisor(row, col, p.color(), out); break;
  case PieceType::Elephant: genElephant(row, col, p.color(), out); break;
  case PieceType::Knight: genKnight(row, col, p.color(), out); break;
  case PieceType::Rook: genRook(row, col, p.color(), out); break;
  case PieceType::Cannon: genCannon(row, col, p.color(), out); break;
  case PieceType::Pawn: genPawn(row, col, p.color(), out); break;
  default: break;
  }
  return out;
}

// =======================================================================
//  Legal move generation
// =======================================================================
std::vector<Move> Board::legalMovesFor(int row, int col) const {
  const Piece &p = at(row, col);
  if (p.empty() || p.color() != side_)
    return {};

  std::vector<Move> pseudo = pseudoMovesFor(row, col);
  std::vector<Move> legal;
  legal.reserve(pseudo.size());

  for (const auto &m : pseudo)
    if (!leavesKingInCheck(m))
      legal.push_back(m);
  return legal;
}

std::vector<Move> Board::legalMoves() const {
  std::vector<Move> all;
  all.reserve(80);
  for (int r = 0; r < ROWS; ++r)
    for (int c = 0; c < COLS; ++c)
      if (!at(r, c).empty() && at(r, c).color() == side_) {
        auto mv = legalMovesFor(r, c);
        all.insert(all.end(), mv.begin(), mv.end());
      }
  return all;
}

// =======================================================================
//  isCheckmate / isStalemate
// =======================================================================
bool Board::isCheckmate() const { return isInCheck() && legalMoves().empty(); }

bool Board::isStalemate() const { return !isInCheck() && legalMoves().empty(); }

// =======================================================================
//  Apply / undo
// =======================================================================
bool Board::applyMove(const Move &m) {
  if (leavesKingInCheck(m))
    return false;
  doMove(m);
  history_.push_back(m);
  return true;
}

void Board::doMove(const Move &m) {
  setAt(m.from(), Piece::Empty);
  setAt(m.to(), m.moved());
  side_ = opposite(side_);
  if (side_ == PieceColor::Red)
    ++fullMove_;
  // Update half-move clock
  if (m.isCapture() || m.moved().type() == PieceType::Pawn)
    halfMove_ = 0;
  else
    ++halfMove_;
}

void Board::undoLast(const Move &m) {
  setAt(m.from(), m.moved());
  setAt(m.to(), m.captured());
  side_ = opposite(side_);
  if (side_ == PieceColor::Black)
    --fullMove_;
  // Note: halfMove_ is not perfectly restored (would need to store it),
  // but for GUI undo this is acceptable.
}

void Board::undoMove() {
  if (history_.empty())
    return;
  Move m = history_.back();
  history_.pop_back();
  undoLast(m);
}

// =======================================================================
//  Slide ray helper  (used by Rook and Cannon)
// =======================================================================
void Board::slideRay(int                r,
                     int                c,
                     int                dr,
                     int                dc,
                     PieceColor         ownColor,
                     std::vector<Move> &out) const {
  Square from{static_cast<int8_t>(r), static_cast<int8_t>(c)};
  Piece  mover = at(r, c);
  int    nr = r + dr, nc = c + dc;
  while (inBounds(nr, nc)) {
    const Piece &target = at(nr, nc);
    Square       to{static_cast<int8_t>(nr), static_cast<int8_t>(nc)};
    if (target.empty()) {
      out.push_back(Move{from, to, mover, Piece::Empty});
    } else {
      if (target.color() != ownColor)
        out.push_back(Move{from, to, mover, target});
      break; // blocked
    }
    nr += dr;
    nc += dc;
  }
}

// =======================================================================
//  Piece-specific generators
// =======================================================================

// --- King ---
void Board::genKing(int                r,
                    int                c,
                    PieceColor         color,
                    std::vector<Move> &out) const {
  static constexpr int dirs[4][2] = {
      { 1,  0},
      {-1,  0},
      { 0,  1},
      { 0, -1}
  };
  Square from{static_cast<int8_t>(r), static_cast<int8_t>(c)};
  Piece  mover = at(r, c);

  for (auto &d : dirs) {
    int nr = r + d[0], nc = c + d[1];
    if (!inBounds(nr, nc))
      continue;
    if (!inPalace(nr, nc, color))
      continue;
    const Piece &target = at(nr, nc);
    if (target.empty() || target.color() != color)
      out.push_back(Move{
          from,
          {static_cast<int8_t>(nr), static_cast<int8_t>(nc)},
          mover,
          target.empty() ? Piece::Empty : target
      });
  }
}

// --- Advisor ---
void Board::genAdvisor(int                r,
                       int                c,
                       PieceColor         color,
                       std::vector<Move> &out) const {
  static constexpr int dirs[4][2] = {
      { 1,  1},
      { 1, -1},
      {-1,  1},
      {-1, -1}
  };
  Square from{static_cast<int8_t>(r), static_cast<int8_t>(c)};
  Piece  mover = at(r, c);

  for (auto &d : dirs) {
    int nr = r + d[0], nc = c + d[1];
    if (!inBounds(nr, nc))
      continue;
    if (!inPalace(nr, nc, color))
      continue;
    const Piece &target = at(nr, nc);
    if (target.empty() || target.color() != color)
      out.push_back(Move{
          from,
          {static_cast<int8_t>(nr), static_cast<int8_t>(nc)},
          mover,
          target.empty() ? Piece::Empty : target
      });
  }
}

// --- Elephant  (2 diagonal, blocked by intervening piece, cannot cross river)
// ---
void Board::genElephant(int                r,
                        int                c,
                        PieceColor         color,
                        std::vector<Move> &out) const {
  // Diagonal jumps of exactly 2; the midpoint must be empty
  static constexpr int leaps[4][4] = {
      { 2,  2,  1,  1}, // {dr, dc, mid_dr, mid_dc}
      { 2, -2,  1, -1},
      {-2,  2, -1,  1},
      {-2, -2, -1, -1},
  };
  Square from{static_cast<int8_t>(r), static_cast<int8_t>(c)};
  Piece  mover = at(r, c);

  for (auto &leap : leaps) {
    int nr = r + leap[0], nc = c + leap[1];
    int mr = r + leap[2], mc = c + leap[3];
    if (!inBounds(nr, nc))
      continue;
    // Cannot cross river
    bool redSide = (color == PieceColor::Red);
    if (redSide && nr < 5)
      continue; // Red stays rows 5-9
    if (!redSide && nr > 4)
      continue; // Black stays rows 0-4
    if (!isEmpty(mr, mc))
      continue; // blocked
    const Piece &target = at(nr, nc);
    if (target.empty() || target.color() != color)
      out.push_back(Move{
          from,
          {static_cast<int8_t>(nr), static_cast<int8_t>(nc)},
          mover,
          target.empty() ? Piece::Empty : target
      });
  }
}

// --- Knight  (1+2 L-shape, first step must not be blocked) ---
void Board::genKnight(int                r,
                      int                c,
                      PieceColor         color,
                      std::vector<Move> &out) const {
  // Each entry: {dr_first, dc_first, dr_final, dc_final}
  static constexpr int leaps[8][4] = {
      {-1,  0, -2, -1},
      {-1,  0, -2,  1},
      { 1,  0,  2, -1},
      { 1,  0,  2,  1},
      { 0, -1, -1, -2},
      { 0, -1,  1, -2},
      { 0,  1, -1,  2},
      { 0,  1,  1,  2},
  };
  Square from{static_cast<int8_t>(r), static_cast<int8_t>(c)};
  Piece  mover = at(r, c);

  for (auto &leap : leaps) {
    int br = r + leap[0], bc = c + leap[1]; // blocking square
    int nr = r + leap[2], nc = c + leap[3]; // destination
    if (!inBounds(nr, nc))
      continue;
    if (!isEmpty(br, bc))
      continue; // horse's leg blocked
    const Piece &target = at(nr, nc);
    if (target.empty() || target.color() != color)
      out.push_back(Move{
          from,
          {static_cast<int8_t>(nr), static_cast<int8_t>(nc)},
          mover,
          target.empty() ? Piece::Empty : target
      });
  }
}

// --- Rook ---
void Board::genRook(int                r,
                    int                c,
                    PieceColor         color,
                    std::vector<Move> &out) const {
  slideRay(r, c, +1, 0, color, out);
  slideRay(r, c, -1, 0, color, out);
  slideRay(r, c, 0, +1, color, out);
  slideRay(r, c, 0, -1, color, out);
}

// --- Cannon  (slides freely, captures by jumping exactly one screen) ---
void Board::genCannon(int                r,
                      int                c,
                      PieceColor         color,
                      std::vector<Move> &out) const {
  Square from{static_cast<int8_t>(r), static_cast<int8_t>(c)};
  Piece  mover = at(r, c);

  static constexpr int dirs[4][2] = {
      { 1,  0},
      {-1,  0},
      { 0,  1},
      { 0, -1}
  };
  for (auto &d : dirs) {
    int  nr = r + d[0], nc = c + d[1];
    bool screenFound = false;

    while (inBounds(nr, nc)) {
      const Piece &target = at(nr, nc);
      Square       to{static_cast<int8_t>(nr), static_cast<int8_t>(nc)};

      if (!screenFound) {
        if (target.empty()) {
          // Normal (non-capture) move
          out.push_back(Move{from, to, mover, Piece::Empty});
        } else {
          screenFound = true; // found the screen piece
        }
      } else {
        if (!target.empty()) {
          // Capture: jump over exactly one screen
          if (target.color() != color)
            out.push_back(Move{from, to, mover, target});
          break; // can't jump over more
        }
      }
      nr += d[0];
      nc += d[1];
    }
  }
}

// --- Pawn ---
void Board::genPawn(int                r,
                    int                c,
                    PieceColor         color,
                    std::vector<Move> &out) const {
  Square from{static_cast<int8_t>(r), static_cast<int8_t>(c)};
  Piece  mover = at(r, c);

  // Red moves up (decreasing row), Black moves down (increasing row)
  bool isRed        = (color == PieceColor::Red);
  int  forward      = isRed ? -1 : +1;
  bool crossedRiver = isRed ? (r <= 4) : (r >= 5);

  // Forward
  int nr = r + forward, nc = c;
  if (inBounds(nr, nc)) {
    const Piece &target = at(nr, nc);
    if (target.empty() || target.color() != color)
      out.push_back(Move{
          from,
          {static_cast<int8_t>(nr), static_cast<int8_t>(nc)},
          mover,
          target.empty() ? Piece::Empty : target
      });
  }
  // Lateral (only after crossing river)
  if (crossedRiver) {
    for (int dc : {-1, +1}) {
      int sc = c + dc;
      if (!inBounds(r, sc))
        continue;
      const Piece &target = at(r, sc);
      if (target.empty() || target.color() != color)
        out.push_back(Move{
            from,
            {static_cast<int8_t>(r), static_cast<int8_t>(sc)},
            mover,
            target.empty() ? Piece::Empty : target
        });
    }
  }
}

} // namespace XiangQi
