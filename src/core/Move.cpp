#include "Move.hpp"
#include <stdexcept>

namespace XiangQi {

const Move Move::Null{};

// -----------------------------------------------------------------------
//  Square helpers
// -----------------------------------------------------------------------
std::string Square::toString() const {
  if (!valid())
    return "??";
  std::string s;
  s += static_cast<char>('a' + col);
  s += static_cast<char>('0' + (9 - row)); // row 9 = rank 0, row 0 = rank 9
  return s;
}

Square Square::fromString(const std::string &s) {
  if (s.size() < 2)
    return {};
  Square sq;
  sq.col   = static_cast<int8_t>(s[0] - 'a');
  int rank = s[1] - '0'; // rank 0..9
  sq.row   = static_cast<int8_t>(9 - rank);
  return sq.valid() ? sq : Square{};
}

// -----------------------------------------------------------------------
//  Move UCCI string, e.g. "h0g2"
//  UCCI uses column letter (a-i) + rank digit (0-9), rank 0 = Red's side
// -----------------------------------------------------------------------
std::string Move::toUCCI() const { return from_.toString() + to_.toString(); }

Move Move::fromUCCI(const std::string &ucci,
                    const Piece       &moved,
                    const Piece       &captured) {
  if (ucci.size() < 4)
    return Move::Null;
  Square from = Square::fromString(ucci.substr(0, 2));
  Square to   = Square::fromString(ucci.substr(2, 2));
  if (!from.valid() || !to.valid())
    return Move::Null;
  return Move{from, to, moved, captured};
}

} // namespace XiangQi
