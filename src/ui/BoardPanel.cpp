#include "BoardPanel.hpp"
#include "../engine/EnginePool.hpp"
#include <algorithm>
#include <imgui.h>

namespace XiangQi {

BoardPanel::BoardPanel(const TextureManager &texMgr)
    : IPanel("Board")
    , renderer_(texMgr) {}

// -----------------------------------------------------------------------
//  syncConfig  – mirror GameSettings into BoardRenderer::Config
// -----------------------------------------------------------------------
void BoardPanel::syncConfig(const GameSettings &s) {
  auto &cfg = renderer_.config();

  cfg.flipBoard       = s.flipBoard;
  cfg.showCoordinates = s.showCoords;
  cfg.pieceScale      = s.pieceScale;

  auto toVec4 = [](const GameSettings::Color4 &c) -> ImVec4 {
    return {c.r, c.g, c.b, c.a};
  };
  cfg.colorSelected = toVec4(s.colSelected);
  cfg.colorLegal    = toVec4(s.colLegal);
  cfg.colorLastMove = toVec4(s.colLastMove);
  cfg.colorCheck    = toVec4(s.colCheck);
}

// -----------------------------------------------------------------------
//  onRender
// -----------------------------------------------------------------------
void BoardPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6.f, 6.f});
  if (!ImGui::Begin(title_.c_str(), &open_, flags)) {
    ImGui::End();
    ImGui::PopStyleVar();
    return;
  }

  syncConfig(ctx.settings);

  // Fit board height to available content area, preserve 9:10 aspect ratio
  ImVec2 avail = ImGui::GetContentRegionAvail();
  float  byH   = avail.y - 4.0f;
  float  byW   = avail.x / (BoardRenderer::PNG_W / BoardRenderer::PNG_H);
  renderer_.config().boardSize = std::max(200.0f, std::min(byH, byW));

  std::optional<std::string> hintMove;
  if (ctx.hint.has_value())
    hintMove = ctx.hint->moveUcci;

  // Allow mouse input only when the current side-to-move is a Human player
  // (i.e. no engine assigned to that side)
  PieceColor stm        = ctx.gameState.sideToMove();
  bool       isRed      = (stm == PieceColor::Red);
  bool       allowInput = (ctx.settings.pool.activeEngineFor(isRed) == nullptr);

  // Analyze snapshot: first engine in pool with non-empty PV lines
  EnginePool                  &pool = ctx.settings.pool;
  static const AnalyzeSnapshot kEmpty{};
  const AnalyzeSnapshot       *snap = &kEmpty;
  for (auto &e : pool.entries) {
    if (e.ctrl && !e.ctrl->analyzeSnapshot().pvLines.empty()) {
      snap = &e.ctrl->analyzeSnapshot();
      break;
    }
  }
  renderer_.render(ctx.gameState, hintMove, *snap, allowInput);

  ImGui::End();
  ImGui::PopStyleVar();
}

} // namespace XiangQi
