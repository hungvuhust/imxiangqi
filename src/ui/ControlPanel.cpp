#include "ControlPanel.hpp"
#include <cstring>
#include <imgui.h>

namespace XiangQi {

ControlPanel::ControlPanel()
    : IPanel("Controls") {
  std::strncpy(
      fenBuf_,
      "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1",
      sizeof(fenBuf_) - 1);
}

void ControlPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  // --- Game actions -------------------------------------------------------
  ImGui::SeparatorText("Game");

  if (ImGui::Button("New Game", {-1, 0}))
    ctx.newGame();

  bool canUndo = ctx.gameState.canUndo() && !ctx.gameState.isGameOver();
  ImGui::BeginDisabled(!canUndo);
  if (ImGui::Button("Undo", {-1, 0}))
    ctx.undoMove();
  ImGui::EndDisabled();

  // --- View ---------------------------------------------------------------
  ImGui::SeparatorText("View");

  bool flip = ctx.settings.flipBoard;
  if (ImGui::Checkbox("Flip board (Black at bottom)", &flip))
    ctx.settings.flipBoard = flip;

  bool coords = ctx.settings.showCoords;
  if (ImGui::Checkbox("Show coordinates", &coords))
    ctx.settings.showCoords = coords;

  // --- Load FEN -----------------------------------------------------------
  ImGui::SeparatorText("Load FEN");

  ImGui::SetNextItemWidth(-1);
  if (ImGui::InputText("##fen",
                       fenBuf_,
                       sizeof(fenBuf_),
                       ImGuiInputTextFlags_EnterReturnsTrue)) {
    ctx.loadFen(fenBuf_);
    fenError_ = false;
  }

  if (ImGui::Button("Load", {-1, 0})) {
    if (std::strlen(fenBuf_) > 0) {
      ctx.loadFen(fenBuf_);
      fenError_ = false;
    } else {
      fenError_ = true;
    }
  }
  if (fenError_)
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Invalid or empty FEN.");

  // Copy current FEN button
  if (ImGui::Button("Copy current FEN", {-1, 0})) {
    std::string fen = ctx.gameState.toFen();
    ImGui::SetClipboardText(fen.c_str());
  }

  ImGui::End();
}

} // namespace XiangQi
