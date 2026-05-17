#include "Piece.hpp"

namespace XiangQi {

const Piece Piece::Empty{};

// -----------------------------------------------------------------------
//  fromFenChar
//  FEN chars used by UCCI / WXF:
//    R r = Rook      N n = Knight   B b = Elephant  A a = Advisor
//    K k = King      C c = Cannon   P p = Pawn
// -----------------------------------------------------------------------
Piece Piece::fromFenChar(char c) {
  PieceColor color =
      (c >= 'A' && c <= 'Z') ? PieceColor::Red : PieceColor::Black;
  char lo = (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c;

  PieceType type = PieceType::None;
  switch (lo) {
  case 'k': type = PieceType::King; break;
  case 'a': type = PieceType::Advisor; break;
  case 'b': type = PieceType::Elephant; break;
  case 'n': type = PieceType::Knight; break;
  case 'r': type = PieceType::Rook; break;
  case 'c': type = PieceType::Cannon; break;
  case 'p': type = PieceType::Pawn; break;
  default: return Piece::Empty;
  }
  return Piece{type, color};
}

// -----------------------------------------------------------------------
//  toFenChar
// -----------------------------------------------------------------------
char Piece::toFenChar() const {
  char base = '.';
  switch (type_) {
  case PieceType::King: base = 'k'; break;
  case PieceType::Advisor: base = 'a'; break;
  case PieceType::Elephant: base = 'b'; break;
  case PieceType::Knight: base = 'n'; break;
  case PieceType::Rook: base = 'r'; break;
  case PieceType::Cannon: base = 'c'; break;
  case PieceType::Pawn: base = 'p'; break;
  default: return '.';
  }
  return (color_ == PieceColor::Red) ? (base - 'a' + 'A') : base;
}

// -----------------------------------------------------------------------
//  namePieceVN
// -----------------------------------------------------------------------
const char *Piece::namePieceVN() const {
  if (color_ == PieceColor::Red) {
    switch (type_) {
    case PieceType::King: return "Tướng";
    case PieceType::Advisor: return "Sĩ";
    case PieceType::Elephant: return "Tượng";
    case PieceType::Knight: return "Mã";
    case PieceType::Rook: return "Xe";
    case PieceType::Cannon: return "Pháo";
    case PieceType::Pawn: return "Tốt";
    default: return "Trống";
    }
  } else {
    switch (type_) {
    case PieceType::King: return "Tướng";
    case PieceType::Advisor: return "Sĩ";
    case PieceType::Elephant: return "Tượng";
    case PieceType::Knight: return "Mã";
    case PieceType::Rook: return "Xe";
    case PieceType::Cannon: return "Pháo";
    case PieceType::Pawn: return "Tốt";
    default: return "Trống";
    }
  }
}

} // namespace XiangQi
