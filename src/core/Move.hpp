#pragma once
#include "Piece.hpp"
#include <cstdint>
#include <string>

namespace XiangQi {

// -----------------------------------------------------------------------
//  Square  – (row, col) coordinate, 0-based
//  Row 0 = Black's back rank (North), Row 9 = Red's back rank (South)
//  Col 0 = 'a' file, Col 8 = 'i' file
// -----------------------------------------------------------------------
struct Square {
  int8_t row = -1;
  int8_t col = -1;

  bool valid() const { return row >= 0 && row < 10 && col >= 0 && col < 9; }

  bool operator==(const Square &o) const {
    return row == o.row && col == o.col;
  }
  bool operator!=(const Square &o) const { return !(*this == o); }

  // "a0" ... "i9"  (WXF-style string)
  std::string   toString() const;
  static Square fromString(const std::string &s);
};

// -----------------------------------------------------------------------
//  Move  – immutable value describing one half-move
// -----------------------------------------------------------------------
class Move {
public:
  Move() = default;
  Move(Square from, Square to, Piece moved, Piece captured = Piece::Empty)
      : from_(from)
      , to_(to)
      , moved_(moved)
      , captured_(captured) {}

  // Accessors
  Square from() const { return from_; }
  Square to() const { return to_; }
  Piece  moved() const { return moved_; }
  Piece  captured() const { return captured_; }

  bool isCapture() const { return !captured_.empty(); }
  bool valid() const { return from_.valid() && to_.valid(); }

  // UCCI coordinate notation, e.g. "h9g7"
  std::string toUCCI() const;
  static Move fromUCCI(const std::string &ucci,
                       const Piece       &moved,
                       const Piece       &captured = Piece::Empty);

  bool operator==(const Move &o) const {
    return from_ == o.from_ && to_ == o.to_;
  }
  bool operator!=(const Move &o) const { return !(*this == o); }

  static const Move Null;

private:
  Square from_{};
  Square to_{};
  Piece  moved_{};
  Piece  captured_{};
};

} // namespace XiangQi
