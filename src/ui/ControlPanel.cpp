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

  // --- Engine -------------------------------------------------------------
  ImGui::SeparatorText("Engine");

  EngineController &activeEng = ctx.activeEngine();
  bool engineReady            = ctx.settings.hasEngine() && activeEng.isReady();

  // Hint button (dùng engine của bên đang đến lượt)
  bool canHint =
      engineReady && ctx.gameState.isPlaying() && !activeEng.isThinking();
  ImGui::BeginDisabled(!canHint);
  if (ImGui::Button("Hint next move", {-1, 0}))
    activeEng.requestHint(ctx.gameState);
  ImGui::EndDisabled();

  if (ctx.hint.has_value()) {
    const auto &h = *ctx.hint;
    ImGui::TextWrapped("Hint: %s", h.moveUcci.c_str());
    if (h.info.depth >= 0)
      ImGui::Text("Depth: %d", h.info.depth);
    if (h.info.hasScoreCp)
      ImGui::Text("Score cp: %d", h.info.scoreCp);
    if (h.info.hasMate)
      ImGui::Text("Mate in: %d", h.info.mate);
    if (!h.info.pv.empty())
      ImGui::TextWrapped("PV: %s", h.info.pv.c_str());
  }

  ImGui::Spacing();

  // Analyze mode toggle (dùng engineRed làm engine phân tích chính)
  bool analyzing  = ctx.engineRed.isAnalyzing();
  bool canAnalyze = ctx.engineRed.isReady() || analyzing;
  ImGui::BeginDisabled(!canAnalyze);
  if (!analyzing) {
    if (ImGui::Button("Start Analysis", {-1, 0}))
      ctx.engineRed.startAnalyze(ctx.gameState);
  } else {
    if (ImGui::Button("Stop Analysis", {-1, 0}))
      ctx.engineRed.stopAnalyze();
  }
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
