#include "SettingsDialog.hpp"

#include <portable-file-dialogs.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <string>
#include <vector>

namespace {

// Colored status dot drawn via ImDrawList
static void statusDot(ImVec2 pos, ImU32 col, float r = 5.f) {
  ImGui::GetWindowDrawList()->AddCircleFilled(pos, r, col);
}

static ImU32 engineStateColor(XiangQi::EngineState s) {
  using namespace XiangQi;
  switch (s) {
  case EngineState::Ready:
  case EngineState::Thinking: return IM_COL32(60, 210, 100, 255); // green
  case EngineState::Error: return IM_COL32(220, 60, 60, 255);     // red
  case EngineState::Stopped: return IM_COL32(120, 120, 120, 255); // gray
  default: return IM_COL32(210, 160, 40, 255);                    // amber
  }
}

static const char *engineStateLabel(XiangQi::EngineState s) {
  using namespace XiangQi;
  switch (s) {
  case EngineState::Ready: return "Ready";
  case EngineState::Thinking: return "Thinking";
  case EngineState::Handshaking: return "Connecting";
  case EngineState::Starting: return "Starting";
  case EngineState::Error: return "Error";
  case EngineState::Stopped: return "Stopped";
  default: return "Unknown";
  }
}

static std::string defaultPickerPath(const std::string &enginePath) {
  if (enginePath.empty())
    return ".";
  std::filesystem::path p(enginePath);
  return p.has_parent_path() ? p.parent_path().string() : enginePath;
}

static bool renderEngineOptions(XiangQi::EngineController &eng) {
  using namespace XiangQi;
  auto &opts = eng.engineOptions();
  if (opts.empty())
    return false;

  ImGui::SeparatorText("Engine Options");
  bool anyDirty = false;

  for (auto &opt : opts) {
    ImGui::PushID(opt.name.c_str());
    switch (opt.type) {
    case EngineOptionType::Check: {
      bool v = opt.checkVal;
      if (ImGui::Checkbox(opt.name.c_str(), &v)) {
        opt.checkVal = v;
        opt.dirty = anyDirty = true;
      }
      break;
    }
    case EngineOptionType::Spin: {
      int v = opt.spinVal;
      if (ImGui::SliderInt(opt.name.c_str(), &v, opt.spinMin, opt.spinMax)) {
        opt.spinVal = v;
        opt.dirty = anyDirty = true;
      }
      break;
    }
    case EngineOptionType::Combo: {
      std::vector<const char *> items;
      for (auto &var : opt.comboVars) items.push_back(var.c_str());
      int curIdx = 0;
      for (int i = 0; i < (int)opt.comboVars.size(); ++i)
        if (opt.comboVars[i] == opt.strVal) {
          curIdx = i;
          break;
        }
      if (ImGui::Combo(opt.name.c_str(),
                       &curIdx,
                       items.data(),
                       (int)items.size()))
        if (curIdx < (int)opt.comboVars.size()) {
          opt.strVal = opt.comboVars[curIdx];
          opt.dirty = anyDirty = true;
        }
      break;
    }
    case EngineOptionType::String: {
      char buf[128] = {};
      std::strncpy(buf, opt.strVal.c_str(), sizeof(buf) - 1);
      if (ImGui::InputText(opt.name.c_str(),
                           buf,
                           sizeof(buf),
                           ImGuiInputTextFlags_EnterReturnsTrue)) {
        opt.strVal = buf;
        opt.dirty = anyDirty = true;
      }
      break;
    }
    case EngineOptionType::Button:
      if (ImGui::Button(opt.name.c_str())) {
        opt.dirty = true;
        eng.sendOption(opt);
      }
      break;
    }
    ImGui::PopID();
  }
  if (anyDirty) {
    ImGui::Spacing();
    if (ImGui::Button("Apply options", {-1, 0}))
      eng.flushDirtyOptions();
  }
  return anyDirty;
}

} // namespace

namespace XiangQi {

static constexpr float NAV_WIDTH = 140.f;

void SettingsDialog::open(int page) {
  open_     = true;
  bufsInit_ = false;
  if (page >= 0)
    currentPage_ = page;
}

void SettingsDialog::syncBuffers(const EnginePool &pool) {
  if (bufsInit_ && bufs_.size() == pool.entries.size())
    return;
  bufs_.resize(pool.entries.size());
  for (int i = 0; i < (int)pool.entries.size(); ++i) {
    auto &e = pool.entries[i];
    std::strncpy(bufs_[i].path,
                 e.settings.path.c_str(),
                 sizeof(EngBufs::path) - 1);
    std::strncpy(bufs_[i].name,
                 e.displayName.c_str(),
                 sizeof(EngBufs::name) - 1);
  }
  bufsInit_ = true;
}

// ---------------------------------------------------------------------------
void SettingsDialog::render(AppContext &ctx) {
  if (!open_)
    return;

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
  ImGui::SetNextWindowSize({720, 560}, ImGuiCond_Appearing);
  ImGui::SetNextWindowSizeConstraints({500, 400}, {1200, 900});

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
  if (!ImGui::Begin("Settings", &open_, flags)) {
    ImGui::End();
    return;
  }

  // ── Left nav sidebar ────────────────────────────────────────────────────
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 8.f});
  ImGui::BeginChild("##nav", {NAV_WIDTH, 0}, ImGuiChildFlags_Borders);
  ImGui::PopStyleVar();

  renderNav();
  ImGui::EndChild();

  ImGui::SameLine();

  // ── Right content area ───────────────────────────────────────────────────
  ImGui::BeginChild("##content", {0, 0}, ImGuiChildFlags_None);

  switch (currentPage_) {
  case 0: pageGeneral(ctx); break;
  case 1: pageEngines(ctx); break;
  case 2: pageAppearance(ctx); break;
  }

  ImGui::EndChild();

  ImGui::End();
}

// ---------------------------------------------------------------------------
void SettingsDialog::renderNav() {
  struct NavItem {
    const char *label;
    const char *icon;
  };
  static const NavItem items[] = {
      {   "General", "  "},
      {   "Engines", "  "},
      {"Appearance", "  "},
  };

  ImGui::Spacing();
  ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, {0.08f, 0.5f});
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4.f, 6.f});

  for (int i = 0; i < 3; ++i) {
    bool active = (currentPage_ == i);

    if (active) {
      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4{0.22f, 0.47f, 0.72f, 1.f});
      ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
                            ImVec4{0.28f, 0.54f, 0.80f, 1.f});
    }

    char label[64];
    std::snprintf(label, sizeof(label), "%s%s", items[i].icon, items[i].label);
    if (ImGui::Selectable(label, active, ImGuiSelectableFlags_None, {0, 32}))
      currentPage_ = i;

    if (active)
      ImGui::PopStyleColor(2);
  }

  ImGui::PopStyleVar(2);

  // Footer: reset button at bottom of nav
  float navH = ImGui::GetContentRegionAvail().y;
  ImGui::SetCursorPosY(ImGui::GetCursorPosY() + navH - 60.f);
  ImGui::Separator();
  ImGui::Spacing();
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {4.f, 4.f});
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.35f, 0.15f, 0.15f, 1.f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4{0.55f, 0.20f, 0.20f, 1.f});
  if (!confirmReset_) {
    if (ImGui::Button("Reset defaults", {-1, 0}))
      confirmReset_ = true;
  } else {
    ImGui::TextColored({1.f, 0.5f, 0.3f, 1.f}, "Sure?");
    ImGui::SameLine();
    if (ImGui::SmallButton("Yes")) { /* handled in content */
      confirmReset_ = false;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("No"))
      confirmReset_ = false;
  }
  ImGui::PopStyleColor(2);
  ImGui::PopStyleVar();
}

// ---------------------------------------------------------------------------
void SettingsDialog::pageGeneral(AppContext &ctx) {
  GameSettings &s = ctx.settings;

  ImGui::SeparatorText("Board");
  ImGui::SliderFloat("Piece scale", &s.pieceScale, 0.5f, 1.0f, "%.2f");
  ImGui::Checkbox("Flip board", &s.flipBoard);
  ImGui::Checkbox("Show coordinates", &s.showCoords);

  ImGui::Spacing();
  ImGui::SeparatorText("Animation");
  ImGui::Checkbox("Animate moves", &s.animateMoves);
  if (s.animateMoves)
    ImGui::SliderFloat("Speed (ms)", &s.animSpeedMs, 40.f, 400.f, "%.0f");

  if (confirmReset_) {
    ImGui::Spacing();
    ImGui::TextColored({1.f, 0.4f, 0.2f, 1.f},
                       "Reset all settings to defaults?");
    if (ImGui::Button("Yes, reset")) {
      ctx.settings.reset();
      bufsInit_     = false;
      confirmReset_ = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
      confirmReset_ = false;
  }
}

// ---------------------------------------------------------------------------
void SettingsDialog::pageEngines(AppContext &ctx) {
  EnginePool &pool = ctx.settings.pool;
  syncBuffers(pool);

  if (ImGui::Button("+ Add Engine", {-1, 0})) {
    pool.addEngine();
    bufsInit_ = false;
    syncBuffers(pool);
  }
  ImGui::Spacing();

  if (pool.entries.empty()) {
    ImGui::Spacing();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.6f, 0.6f, 0.6f, 1.f});
    ImGui::TextWrapped("No engines configured. Add one to get started.");
    ImGui::PopStyleColor();
    return;
  }

  ImGui::BeginChild("##engine-list", {0, 0}, false);
  for (int i = 0; i < (int)pool.entries.size(); ++i) {
    ImGui::PushID(i);
    bool removed = renderEngineCard(i, ctx);
    ImGui::PopID();
    if (removed)
      break;
    ImGui::Spacing();
  }
  ImGui::EndChild();
}

// ---------------------------------------------------------------------------
void SettingsDialog::pageAppearance(AppContext &ctx) {
  GameSettings &s = ctx.settings;

  ImGui::SeparatorText("Theme");
  int theme = static_cast<int>(s.theme);
  if (ImGui::RadioButton("Classic", &theme, 0))
    s.theme = BoardTheme::Classic;
  ImGui::SameLine();
  if (ImGui::RadioButton("Simple", &theme, 1))
    s.theme = BoardTheme::Simple;

  ImGui::Spacing();
  ImGui::SeparatorText("Highlight Colors");

  auto editColor = [](const char *label, GameSettings::Color4 &c) {
    float arr[4] = {c.r, c.g, c.b, c.a};
    if (ImGui::ColorEdit4(label,
                          arr,
                          ImGuiColorEditFlags_AlphaBar |
                              ImGuiColorEditFlags_NoInputs))
      c = {arr[0], arr[1], arr[2], arr[3]};
  };
  editColor("Selected piece", s.colSelected);
  editColor("Legal move", s.colLegal);
  editColor("Last move", s.colLastMove);
  editColor("Check", s.colCheck);
}

// ---------------------------------------------------------------------------
bool SettingsDialog::renderEngineCard(int idx, AppContext &ctx) {
  EnginePool        &pool = ctx.settings.pool;
  EnginePool::Entry &e    = pool.entries[idx];
  EngineController  &eng  = *e.ctrl;
  EngBufs           &buf  = bufs_[idx];

  // ── Card border ─────────────────────────────────────────────────────────
  float cardW = ImGui::GetContentRegionAvail().x;
  ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 6.f);
  ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4{0.14f, 0.14f, 0.16f, 1.f});
  ImGui::BeginChild("##card",
                    {cardW, 0},
                    ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();

  // ── Card header row ──────────────────────────────────────────────────────
  // Status dot
  ImVec2 dotPos = {ImGui::GetCursorScreenPos().x + 8.f,
                   ImGui::GetCursorScreenPos().y +
                       ImGui::GetTextLineHeight() * 0.5f + 6.f};
  ImGui::Dummy({18.f, 0.f}); // space for dot
  ImGui::SameLine();

  // Engine name (bold-ish via larger text or just regular)
  std::string nameLabel =
      e.displayName.empty() ? ("Engine " + std::to_string(idx)) : e.displayName;
  ImGui::Text("%s", nameLabel.c_str());

  // Draw status dot on top of dummy
  statusDot(dotPos, engineStateColor(eng.state()), 5.f);

  // State badge (right-aligned)
  const char *stateStr = engineStateLabel(eng.state());
  float       badgeX   = cardW - ImGui::CalcTextSize(stateStr).x - 80.f;
  ImGui::SameLine(badgeX);
  ImVec4 stateCol = (eng.state() == EngineState::Error)
                        ? ImVec4{1.f, 0.4f, 0.4f, 1.f}
                        : (eng.isRunning() ? ImVec4{0.4f, 0.85f, 0.55f, 1.f}
                                           : ImVec4{0.6f, 0.6f, 0.6f, 1.f});
  ImGui::TextColored(stateCol, "%s", stateStr);

  // Delete button (top-right)
  ImGui::SameLine(cardW - 30.f);
  ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.f, 0.f, 0.f, 0.f});
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                        ImVec4{0.6f, 0.15f, 0.15f, 1.f});
  if (ImGui::SmallButton("✕##del"))
    deleteConfirm_ = idx;
  ImGui::PopStyleColor(2);

  ImGui::Separator();

  // Delete confirm row
  if (deleteConfirm_ == idx) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.f, 0.5f, 0.3f, 1.f});
    ImGui::Text("Remove this engine?");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::SmallButton("Remove")) {
      pool.removeEngine(idx);
      bufsInit_ = false;
      syncBuffers(pool);
      deleteConfirm_ = -1;
      ImGui::EndChild();
      return true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("Keep"))
      deleteConfirm_ = -1;
    ImGui::Separator();
  }

  // ── Display name ─────────────────────────────────────────────────────────
  ImGui::SetNextItemWidth(-1.f);
  if (ImGui::InputText("##name",
                       buf.name,
                       sizeof(buf.name),
                       ImGuiInputTextFlags_EnterReturnsTrue)) {
    e.displayName   = buf.name;
    e.settings.name = buf.name;
    pool.ensureUniqueName(idx);
    std::strncpy(buf.name, e.displayName.c_str(), sizeof(buf.name) - 1);
  }
  ImGui::SetItemTooltip("Display name");

  // ── Engine path row ───────────────────────────────────────────────────────
  ImGui::SetNextItemWidth(-90.f);
  if (ImGui::InputText("##path", buf.path, sizeof(buf.path)))
    e.settings.path = buf.path;
  ImGui::SetItemTooltip("Path to engine binary");
  ImGui::SameLine();
  if (ImGui::Button("Browse", {82, 0})) {
    pfd::open_file picker("Select engine binary",
                          defaultPickerPath(e.settings.path),
                          {"All Files", "*"},
                          pfd::opt::none);
    auto           files = picker.result();
    if (!files.empty()) {
      std::strncpy(buf.path, files[0].c_str(), sizeof(buf.path) - 1);
      e.settings.path = buf.path;
      if (e.displayName.empty()) {
        std::string stem = std::filesystem::path(files[0]).stem().string();
        std::strncpy(buf.name, stem.c_str(), sizeof(buf.name) - 1);
        e.displayName   = buf.name;
        e.settings.name = buf.name;
        pool.ensureUniqueName(idx);
        std::strncpy(buf.name, e.displayName.c_str(), sizeof(buf.name) - 1);
      }
    }
  }

  // ── Search parameters (2-column grid) ────────────────────────────────────
  ImGui::Spacing();
  if (ImGui::BeginTable("##params", 2, ImGuiTableFlags_None)) {
    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderInt("Depth##d", &e.settings.depth, 1, 30);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderInt("Time (ms)##t", &e.settings.timeMs, 100, 10000);

    ImGui::TableNextColumn();
    ImGui::SetNextItemWidth(-1.f);
    ImGui::SliderInt("MultiPV##m", &e.settings.multiPv, 1, 5);
    ImGui::SetItemTooltip("Number of best lines to analyse simultaneously");

    ImGui::TableNextColumn();
    ImGui::Checkbox("Pondering##p", &e.settings.ponder);
    ImGui::SetItemTooltip("Engine thinks during opponent's turn");

    ImGui::EndTable();
  }

  // ── Engine ID + error ─────────────────────────────────────────────────────
  if (!eng.engineIdName().empty()) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.55f, 0.55f, 0.55f, 1.f});
    ImGui::Text("ID: %s", eng.engineIdName().c_str());
    ImGui::PopStyleColor();
  }
  if (!eng.lastError().empty())
    ImGui::TextColored({1.f, 0.35f, 0.35f, 1.f},
                       "⚠  %s",
                       eng.lastError().c_str());

  // ── Control buttons ───────────────────────────────────────────────────────
  ImGui::Spacing();
  bool canStart = !eng.isRunning();
  bool canStop  = eng.isRunning();
  ImGui::BeginDisabled(!canStart);
  if (ImGui::Button("Start", {70, 0}))
    eng.start();
  ImGui::EndDisabled();
  ImGui::SameLine();
  ImGui::BeginDisabled(!canStop);
  if (ImGui::Button("Stop", {70, 0}))
    eng.stop();
  ImGui::EndDisabled();
  ImGui::SameLine();
  if (ImGui::Button("Restart", {70, 0}))
    eng.restart();

  // ── Advanced engine options (collapsible) ─────────────────────────────────
  if (!eng.engineOptions().empty()) {
    ImGui::Spacing();
    renderEngineOptions(eng);
  }

  ImGui::Spacing();
  ImGui::EndChild();
  return false;
}

} // namespace XiangQi
