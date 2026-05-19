#pragma once
#include "Board.hpp"
#include "Move.hpp"
#include <string>

namespace XiangQi {

// Chuyển nước đi sang ký hiệu tiếng Việt chuẩn cờ tướng.
// Ví dụ: "Xe 9 tấn 1", "Mã 8 tấn 7", "Pháo 2 bình 5"
//
// Quy tắc đánh số cột:
//   Đỏ  : 1 (trái) … 9 (phải) → col_num = col + 1
//   Đen : 1 (phải) … 9 (trái) → col_num = 9 - col
//
// Board layout: row 0 = Black back rank (top), row 9 = Red back rank (bottom)
//   Đỏ  tấn = row giảm (di lên màn hình)
//   Đen  tấn = row tăng (di xuống màn hình)
//
// `board` là trạng thái TRƯỚC khi áp dụng move (để lấy piece tại from).
std::string moveToVN(const Move &m, const Board &boardBefore);

} // namespace XiangQi
