#include "StatusPanel.hpp"
#include <imgui.h>

namespace XiangQi {

StatusPanel::StatusPanel()
    : IPanel("Status") {}

void StatusPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  const GameState &gs = ctx.gameState;

  // --- Game result --------------------------------------------------------
  switch (gs.status()) {
  case GameStatus::RedWins:
    ImGui::TextColored({1.f, 0.35f, 0.1f, 1.f}, "Red (South) wins!");
    break;
  case GameStatus::BlackWins:
    ImGui::TextColored({0.5f, 0.65f, 1.f, 1.f}, "Black (North) wins!");
    break;
  case GameStatus::Draw:
    ImGui::TextColored({0.8f, 0.8f, 0.2f, 1.f}, "Draw");
    break;
  case GameStatus::Playing: {
    // Side to move indicator
    bool   isRed = (gs.sideToMove() == PieceColor::Red);
    ImVec4 col =
        isRed ? ImVec4{1.f, 0.4f, 0.2f, 1.f} : ImVec4{0.55f, 0.7f, 1.f, 1.f};
    ImGui::TextColored(col, "%s to move", isRed ? "Red" : "Black");

    if (gs.board().isInCheck())
      ImGui::TextColored({1.f, 0.15f, 0.1f, 1.f}, "  CHECK!");
    break;
  }
  }

  ImGui::Separator();

  // --- Move counter -------------------------------------------------------
  int full = gs.board().fullMoveNum();
  int half = gs.board().halfMoveClock();
  ImGui::Text("Move: %d  |  Half-clock: %d", full, half);

  // --- Material count (quick display) ------------------------------------
  ImGui::Separator();
  ImGui::Text("Pieces on board:");

  int redCount = 0, blackCount = 0;
  for (int r = 0; r < Board::ROWS; ++r)
    for (int c = 0; c < Board::COLS; ++c) {
      const Piece &p = gs.board().at(r, c);
      if (!p.empty()) {
        if (p.color() == PieceColor::Red)
          ++redCount;
        else
          ++blackCount;
      }
    }
  ImGui::TextColored({1.f, 0.45f, 0.2f, 1.f}, "  Red:   %d", redCount);
  ImGui::TextColored({0.55f, 0.7f, 1.f, 1.f}, "  Black: %d", blackCount);

  ImGui::End();
}

} // namespace XiangQi
