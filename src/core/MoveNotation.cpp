#include "MoveNotation.hpp"
#include <algorithm>
#include <cstdio>

namespace XiangQi {

// Số cột theo góc nhìn của bên đang đi:
//   col 0 = file 'a' (trái màn hình), col 8 = file 'i' (phải màn hình)
//
//   Đỏ  (ngồi dưới, nhìn lên): phải → trái = 1 → 9
//        → col 8 = 1, col 0 = 9  →  số = 9 - col
//   Đen (ngồi trên, nhìn xuống): trái → phải = 1 → 9
//        → col 0 = 1, col 8 = 9  →  số = col + 1
static int colNum(int col, PieceColor side) {
  return side == PieceColor::Red ? 9 - col : col + 1;
}

std::string moveToVN(const Move &m, const Board &boardBefore) {
  const Piece &piece = m.moved();
  if (piece.empty())
    return m.toUCCI();

  const PieceColor side = piece.color();
  const PieceType  type = piece.type();
  const int        fr = m.from().row, fc = m.from().col;
  const int        tr = m.to().row, tc = m.to().col;

  // --- Phát hiện 2 quân cùng loại cùng cột (tiền/hậu) ---
  // Tìm các quân cùng type + color trên cùng cột fc
  std::vector<int> sameColRows;
  for (int r = 0; r < Board::ROWS; ++r) {
    const Piece &p = boardBefore.at(r, fc);
    if (!p.empty() && p.type() == type && p.color() == side)
      sameColRows.push_back(r);
  }
  // Sắp xếp theo chiều tấn (đỏ: row nhỏ = tiền; đen: row lớn = tiền)
  std::sort(sameColRows.begin(), sameColRows.end());

  std::string prefix; // "" hoặc "Tiền " / "Hậu "
  if (sameColRows.size() >= 2) {
    // Đỏ tấn theo hướng row giảm → row nhỏ hơn = tiền
    // Đen tấn theo hướng row tăng → row lớn hơn = tiền
    int frontRow = (side == PieceColor::Red)
                       ? sameColRows.front() // nhỏ nhất = gần địch nhất
                       : sameColRows.back(); // lớn nhất = gần địch nhất
    prefix       = (fr == frontRow) ? "Tiền " : "Hậu ";
  }

  // Tên quân
  const char *name = piece.namePieceVN();

  // Số cột xuất phát
  int fromColNum = colNum(fc, side);

  // --- Xác định hướng và bước đi ---
  const char *dir;
  int         step;

  if (fr == tr) {
    // Di chuyển ngang → bình
    dir  = "bình";
    step = colNum(tc, side); // đích theo số cột
  } else if (type == PieceType::Rook || type == PieceType::Cannon ||
             type == PieceType::King || type == PieceType::Pawn) {
    // Các quân đi thẳng: step = số ô di chuyển
    bool advancing = (side == PieceColor::Red) ? (tr < fr) : (tr > fr);
    dir            = advancing ? "tấn" : "thoái";
    step           = std::abs(tr - fr);
  } else {
    // Mã, Tượng, Sĩ: đi chéo → step = số cột đích
    bool advancing = (side == PieceColor::Red) ? (tr < fr) : (tr > fr);
    dir            = advancing ? "tấn" : "thoái";
    step           = colNum(tc, side);
  }

  // --- Ghép chuỗi ---
  char buf[64];
  std::snprintf(buf,
                sizeof(buf),
                "%s%s %d %s %d",
                prefix.c_str(),
                name,
                fromColNum,
                dir,
                step);
  return buf;
}

} // namespace XiangQi
