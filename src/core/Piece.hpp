#pragma once
#include <cstdint>
#include <string>

namespace XiangQi {

// -----------------------------------------------------------------------
//  PieceColor
// -----------------------------------------------------------------------
enum class PieceColor : uint8_t {
  Red   = 0, // South / Hồng / 红
  Black = 1, // North / Hắc  / 黑
  None  = 2,
};

inline PieceColor opposite(PieceColor c) {
  return c == PieceColor::Red ? PieceColor::Black : PieceColor::Red;
}

inline const char *colorName(PieceColor c) {
  switch (c) {
  case PieceColor::Red: return "Red";
  case PieceColor::Black: return "Black";
  default: return "None";
  }
}

// -----------------------------------------------------------------------
//  PieceType
// -----------------------------------------------------------------------
enum class PieceType : uint8_t {
  King     = 0, // 將/帥  K/k
  Advisor  = 1, // 士     A/a
  Elephant = 2, // 象/相  B/b  (Bishop alias)
  Knight   = 3, // 馬     N/n
  Rook     = 4, // 車     R/r
  Cannon   = 5, // 砲     C/c
  Pawn     = 6, // 兵/卒  P/p
  None     = 7,
};

// -----------------------------------------------------------------------
//  Piece  – value type, cheap to copy
// -----------------------------------------------------------------------
class Piece {
public:
  // Constructors
  Piece() = default;
  Piece(PieceType type, PieceColor color)
      : type_(type)
      , color_(color) {}

  // Factory: parse a single FEN character
  static Piece fromFenChar(char c);

  // Accessors
  PieceType  type() const { return type_; }
  PieceColor color() const { return color_; }
  bool       empty() const { return type_ == PieceType::None; }

  // Convert back to FEN char (uppercase = Red, lowercase = Black)
  char toFenChar() const;

  // Human-readable name in Vietnamese
  const char *namePieceVN() const;

  // Equality
  bool operator==(const Piece &o) const {
    return type_ == o.type_ && color_ == o.color_;
  }
  bool operator!=(const Piece &o) const { return !(*this == o); }

  // Null / empty sentinel
  static const Piece Empty;

private:
  PieceType  type_  = PieceType::None;
  PieceColor color_ = PieceColor::None;
};

} // namespace XiangQi
