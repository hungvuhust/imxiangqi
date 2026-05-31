#include "ControlPanel.hpp"
#include "../engine/EnginePool.hpp"
#include "../font_awesome_icons.hpp"
#include <cstring>
#include <imgui.h>
#include <vector>

namespace XiangQi {

ControlPanel::ControlPanel()
    : IPanel("Controls") {
  std::strncpy(
      fenBuf_,
      "rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR w - - 0 1",
      sizeof(fenBuf_) - 1);
}

// Helper: render a "side assignment" row (Red or Black)
// Returns true if assignment changed.
static void
renderSideAssignment(const char *label, int &idxRef, EnginePool &pool) {
  auto names = pool.comboNames(); // ["Human", eng0, eng1, ...]
  std::vector<const char *> items;
  items.reserve(names.size());
  for (auto &n : names) items.push_back(n.c_str());

  int combo = pool.idxToCombo(idxRef); // -1→0, 0→1, ...
  ImGui::SetNextItemWidth(-1);
  if (ImGui::Combo(label, &combo, items.data(), (int)items.size()))
    idxRef = pool.comboToIdx(combo);
}

void ControlPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  // --- Players ------------------------------------------------------------
  ImGui::SeparatorText("Players");

  EnginePool &pool = ctx.settings.pool;

  ImGui::TextUnformatted("Red (South)");
  renderSideAssignment("##redPlayer", pool.redIdx, pool);

  ImGui::TextUnformatted("Black (North)");
  renderSideAssignment("##blackPlayer", pool.blackIdx, pool);

  // --- Game actions -------------------------------------------------------
  ImGui::SeparatorText("Game");

  if (ImGui::Button(ICON_FA_FLAG " New Game", {-1, 0}))
    ctx.newGame();

  bool canUndo = ctx.gameState.canUndo() && !ctx.gameState.isGameOver();
  ImGui::BeginDisabled(!canUndo);
  if (ImGui::Button(ICON_FA_ARROW_LEFT " Undo", {-1, 0}))
    ctx.undoMove();
  ImGui::EndDisabled();

  // --- Engine -------------------------------------------------------------
  ImGui::SeparatorText("Engine");

  EngineController *activeEng   = ctx.activeEngine();
  bool              engineReady = activeEng && activeEng->isReady();

  // Hint: dùng bất kỳ engine nào sẵn sàng (không cần phải là bên đang đi)
  EngineController *hintEng = nullptr;
  for (auto &e : ctx.settings.pool.entries)
    if (e.ctrl && e.ctrl->isReady()) {
      hintEng = e.ctrl.get();
      break;
    }
  bool canHint = hintEng && ctx.gameState.isPlaying();
  ImGui::BeginDisabled(!canHint);
  if (ImGui::Button(ICON_FA_LIGHTBULB " Hint", {-1, 0}))
    hintEng->requestHint(ctx.gameState);
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

  // Analyze mode – dùng engine Red (hoặc engine đầu tiên trong pool)
  EngineController *analyzeEng = pool.redEngine();
  if (!analyzeEng && !pool.entries.empty())
    analyzeEng = pool.entries[0].ctrl.get();

  if (analyzeEng) {
    bool analyzing  = analyzeEng->isAnalyzing();
    bool canAnalyze = analyzeEng->isReady() || analyzing;
    ImGui::BeginDisabled(!canAnalyze);
    if (!analyzing) {
      if (ImGui::Button(ICON_FA_PLAY " Start Analysis", {-1, 0}))
        analyzeEng->startAnalyze(ctx.gameState);
    } else {
      if (ImGui::Button(ICON_FA_STOP " Stop Analysis", {-1, 0}))
        analyzeEng->stopAnalyze();
    }
    ImGui::EndDisabled();
  }

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

  if (ImGui::Button(ICON_FA_UPLOAD " Load FEN", {-1, 0})) {
    if (std::strlen(fenBuf_) > 0) {
      ctx.loadFen(fenBuf_);
      fenError_ = false;
    } else {
      fenError_ = true;
    }
  }
  if (fenError_)
    ImGui::TextColored({1, 0.3f, 0.3f, 1}, "Invalid or empty FEN.");

  if (ImGui::Button(ICON_FA_COPY " Copy FEN", {-1, 0}))
    ImGui::SetClipboardText(ctx.gameState.toFen().c_str());

  ImGui::End();
}

} // namespace XiangQi
