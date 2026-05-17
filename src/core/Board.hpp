#pragma once
#include "Move.hpp"
#include "Piece.hpp"
#include <array>
#include <optional>
#include <string>
#include <vector>

namespace XiangQi {

// -----------------------------------------------------------------------
//  Board  – Immutable grid + move-generation + undo stack
//
//  Layout (row 0 = Black's back rank, row 9 = Red's back rank):
//
//    row 0  :  r n b a k a b n r   (Black)
//    row 2  :  . c . . . . . c .
//    row 3  :  p . p . p . p . p
//    -------- river --------
//    row 6  :  P . P . P . P . P
//    row 7  :  . C . . . . . C .
//    row 9  :  R N B A K A B N R   (Red)
//
//  Palace bounds:
//    Red   : rows 7-9, cols 3-5
//    Black : rows 0-2, cols 3-5
// -----------------------------------------------------------------------
class Board {
public:
  static constexpr int ROWS = 10;
  static constexpr int COLS = 9;

  // Standard UCCI start position
  static constexpr const char *INITIAL_FEN =
      "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1";

  // ------------------------------------------------------------------
  //  Construction
  // ------------------------------------------------------------------
  Board();
  explicit Board(const std::string &fen);

  // Reset to the standard start position
  void reset();

  // Load / dump FEN
  void        loadFen(const std::string &fen);
  std::string toFen() const;

  // ------------------------------------------------------------------
  //  Grid access  (row 0..9, col 0..8)
  // ------------------------------------------------------------------
  const Piece &at(int row, int col) const;
  const Piece &at(Square sq) const { return at(sq.row, sq.col); }

  bool isEmpty(int row, int col) const { return at(row, col).empty(); }
  bool isEmpty(Square sq) const { return at(sq).empty(); }

  bool inBounds(int row, int col) const;
  bool inBounds(Square sq) const { return inBounds(sq.row, sq.col); }

  // ------------------------------------------------------------------
  //  Game state
  // ------------------------------------------------------------------
  PieceColor sideToMove() const { return side_; }
  int        halfMoveClock() const { return halfMove_; }
  int        fullMoveNum() const { return fullMove_; }

  // ------------------------------------------------------------------
  //  Move application / undo
  // ------------------------------------------------------------------

  // Returns false and does nothing if the move is illegal
  bool applyMove(const Move &m);

  // Undo the last applied move; no-op if history is empty
  void undoMove();

  // ------------------------------------------------------------------
  //  Move generation
  // ------------------------------------------------------------------

  // All legal moves for a single piece at (row,col)
  std::vector<Move> legalMovesFor(int row, int col) const;
  std::vector<Move> legalMovesFor(Square sq) const {
    return legalMovesFor(sq.row, sq.col);
  }

  // All legal moves for the side to move
  std::vector<Move> legalMoves() const;

  // ------------------------------------------------------------------
  //  Position queries
  // ------------------------------------------------------------------
  bool isInCheck() const; // current side
  bool isInCheck(PieceColor side) const;
  bool isCheckmate() const;
  bool isStalemate() const; // Xiangqi: stalemate = loss

  Square findKing(PieceColor side) const;

  // ------------------------------------------------------------------
  //  History
  // ------------------------------------------------------------------
  const std::vector<Move> &history() const { return history_; }
  bool                     canUndo() const { return !history_.empty(); }

private:
  // ------------------------------------------------------------------
  //  Internal grid
  // ------------------------------------------------------------------
  std::array<std::array<Piece, COLS>, ROWS> cells_;

  PieceColor side_     = PieceColor::Red;
  int        halfMove_ = 0;
  int        fullMove_ = 1;

  std::vector<Move> history_;

  // ------------------------------------------------------------------
  //  Internal helpers
  // ------------------------------------------------------------------
  Piece &atMut(int row, int col);
  Piece &atMut(Square sq) { return atMut(sq.row, sq.col); }

  void setAt(int row, int col, const Piece &p);
  void setAt(Square sq, const Piece &p) { setAt(sq.row, sq.col, p); }

  // Apply/undo without legality check (for internal use + perft)
  void doMove(const Move &m);
  void undoLast(const Move &m);

  // Check if a move leaves the moving side's king attacked
  bool leavesKingInCheck(const Move &m) const;

  // Pseudo-legal generators (ignore king safety)
  std::vector<Move> pseudoMovesFor(int row, int col) const;
  void genKing(int r, int c, PieceColor color, std::vector<Move> &out) const;
  void genAdvisor(int r, int c, PieceColor color, std::vector<Move> &out) const;
  void
  genElephant(int r, int c, PieceColor color, std::vector<Move> &out) const;
  void genKnight(int r, int c, PieceColor color, std::vector<Move> &out) const;
  void genRook(int r, int c, PieceColor color, std::vector<Move> &out) const;
  void genCannon(int r, int c, PieceColor color, std::vector<Move> &out) const;
  void genPawn(int r, int c, PieceColor color, std::vector<Move> &out) const;

  // Palace: King & Advisor must stay inside
  bool inPalace(int r, int c, PieceColor color) const;

  // Flying generals rule: two kings facing each other with no piece in between
  bool flyingGenerals() const;

  // Is square (r,c) attacked by any piece of `byColor`?
  bool isAttacked(int r, int c, PieceColor byColor) const;

  // Slide helper: add moves along a ray, stop at first occupant
  void slideRay(int                r,
                int                c,
                int                dr,
                int                dc,
                PieceColor         ownColor,
                std::vector<Move> &out) const;
};

} // namespace XiangQi
