#include "SettingsDialog.hpp"

#include <portable-file-dialogs.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <imgui.h>
#include <string>
#include <vector>

namespace {

std::string defaultPickerPath(const std::string &enginePath) {
  if (enginePath.empty())
    return ".";
  std::filesystem::path p(enginePath);
  return p.has_parent_path() ? p.parent_path().string() : enginePath;
}

// Render engine dynamic options list; returns true if any dirty option
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

static constexpr const char *POPUP_ID = "Settings##modal";

void SettingsDialog::open() {
  pendingOpen_ = true;
  bufsInit_    = false;
}

// -----------------------------------------------------------------------
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

// -----------------------------------------------------------------------
void SettingsDialog::render(AppContext &ctx) {
  if (pendingOpen_) {
    ImGui::OpenPopup(POPUP_ID);
    pendingOpen_ = false;
  }

  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
  ImGui::SetNextWindowSize({680, 560}, ImGuiCond_Appearing);

  if (!ImGui::BeginPopupModal(POPUP_ID, nullptr, ImGuiWindowFlags_NoResize))
    return;

  if (ImGui::BeginTabBar("##settings_tabs")) {
    if (ImGui::BeginTabItem("General")) {
      tabGeneral(ctx);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Engine")) {
      tabEngine(ctx);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Board")) {
      tabBoard(ctx);
      ImGui::EndTabItem();
    }
    if (ImGui::BeginTabItem("Colors")) {
      tabColors(ctx);
      ImGui::EndTabItem();
    }
    ImGui::EndTabBar();
  }

  ImGui::Separator();

  if (!confirmReset_) {
    if (ImGui::Button("Reset to defaults"))
      confirmReset_ = true;
  } else {
    ImGui::TextColored({1.f, 0.4f, 0.2f, 1.f}, "Reset all settings?");
    ImGui::SameLine();
    if (ImGui::Button("Yes")) {
      ctx.settings.reset();
      bufsInit_     = false;
      confirmReset_ = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel##reset"))
      confirmReset_ = false;
  }

  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 50);
  if (ImGui::Button("Close", {55, 0}))
    ImGui::CloseCurrentPopup();

  ImGui::EndPopup();
}

// -----------------------------------------------------------------------
void SettingsDialog::tabGeneral(AppContext &ctx) {
  ImGui::SeparatorText("Info");
  ImGui::TextWrapped(
      "Configure player assignments in the Controls panel "
      "(left sidebar). Use the Engine tab to manage engine "
      "instances.");
  ImGui::Spacing();
  ImGui::SeparatorText("Board");
  ImGui::SliderFloat("Piece scale",
                     &ctx.settings.pieceScale,
                     0.5f,
                     1.0f,
                     "%.2f");
  ImGui::Checkbox("Flip board", &ctx.settings.flipBoard);
  ImGui::Checkbox("Show coordinates", &ctx.settings.showCoords);
  ImGui::Checkbox("Animate moves", &ctx.settings.animateMoves);
  if (ctx.settings.animateMoves)
    ImGui::SliderFloat("Anim speed (ms)",
                       &ctx.settings.animSpeedMs,
                       40.f,
                       400.f,
                       "%.0f");
}

// -----------------------------------------------------------------------
void SettingsDialog::tabEngine(AppContext &ctx) {
  EnginePool &pool = ctx.settings.pool;
  syncBuffers(pool);

  // New Engine button
  if (ImGui::Button("+ New Engine", {-1, 0})) {
    pool.addEngine();
    bufsInit_ = false; // mark dirty so syncBuffers will re-init all slots
    syncBuffers(pool); // re-sync immediately so bufs_ matches pool this frame
  }

  ImGui::Spacing();

  if (pool.entries.empty()) {
    ImGui::TextDisabled(
        "No engines configured. Click '+ New Engine' to add one.");
    return;
  }

  // Scrollable child for N engine panels
  ImGui::BeginChild("##engine-list", {0, 0}, false);
  for (int i = 0; i < (int)pool.entries.size(); ++i) {
    ImGui::PushID(i);
    bool removed = renderOneEngine(i, ctx);
    ImGui::PopID();
    if (removed) break; // bufs_ was invalidated; skip remaining this frame
    ImGui::Spacing();
  }
  ImGui::EndChild();
}

// -----------------------------------------------------------------------
// Returns true if the engine was removed (caller must break render loop).
bool SettingsDialog::renderOneEngine(int idx, AppContext &ctx) {
  EnginePool        &pool = ctx.settings.pool;
  EnginePool::Entry &e    = pool.entries[idx];
  EngineController  &eng  = *e.ctrl;
  EngBufs           &buf  = bufs_[idx];

  // Header with collapse + delete
  std::string headerLabel =
      std::string("Engine: ") + (e.displayName.empty()
                                     ? ("Engine " + std::to_string(idx))
                                     : e.displayName);
  bool open = ImGui::CollapsingHeader(headerLabel.c_str(),
                                      ImGuiTreeNodeFlags_DefaultOpen);

  // Delete button (right-aligned)
  ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30);
  if (ImGui::SmallButton("X##del"))
    deleteConfirm_ = idx;

  if (deleteConfirm_ == idx) {
    ImGui::TextColored({1.f, 0.4f, 0.2f, 1.f}, "Delete this engine?");
    ImGui::SameLine();
    if (ImGui::SmallButton("Yes##delyes")) {
      pool.removeEngine(idx);
      bufsInit_      = false;
      syncBuffers(pool); // resize bufs_ immediately so it stays valid
      deleteConfirm_ = -1;
      return true; // idx is now invalid; caller must break loop
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("No##delno"))
      deleteConfirm_ = -1;
  }

  if (!open)
    return false;

  // Display name
  if (ImGui::InputText("Display name",
                       buf.name,
                       sizeof(buf.name),
                       ImGuiInputTextFlags_EnterReturnsTrue)) {
    e.displayName   = buf.name;
    e.settings.name = buf.name;
    pool.ensureUniqueName(idx);
    std::strncpy(buf.name, e.displayName.c_str(), sizeof(buf.name) - 1);
  }

  // Engine path + browse
  ImGui::SetNextItemWidth(-110);
  if (ImGui::InputText("Engine path", buf.path, sizeof(buf.path)))
    e.settings.path = buf.path;
  ImGui::SameLine();
  if (ImGui::Button("Browse...", {100, 0})) {
    pfd::open_file picker("Select engine binary",
                          defaultPickerPath(e.settings.path),
                          {"All Files", "*"},
                          pfd::opt::none);
    auto           files = picker.result();
    if (!files.empty()) {
      std::strncpy(buf.path, files[0].c_str(), sizeof(buf.path) - 1);
      e.settings.path = buf.path;
      // Auto-fill display name from filename if still empty
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

  ImGui::SliderInt("Depth", &e.settings.depth, 1, 30);
  ImGui::SliderInt("Time (ms)", &e.settings.timeMs, 100, 10000);
  ImGui::SliderInt("MultiPV", &e.settings.multiPv, 1, 5);
  ImGui::SetItemTooltip("Number of best lines to analyse simultaneously");
  ImGui::Checkbox("Pondering", &e.settings.ponder);
  ImGui::SetItemTooltip("Engine thinks during opponent's turn");

  // Runtime status
  ImGui::Spacing();
  ImGui::Text("State: %s  Protocol: %s  %s",
              toString(eng.state()),
              toString(eng.protocol()),
              eng.isRunning() ? "Running" : "Stopped");
  if (!eng.engineIdName().empty())
    ImGui::Text("ID: %s", eng.engineIdName().c_str());
  if (!eng.lastError().empty())
    ImGui::TextColored({1.f, 0.35f, 0.35f, 1.f},
                       "Error: %s",
                       eng.lastError().c_str());

  if (ImGui::Button("Start", {80, 0}))
    eng.start();
  ImGui::SameLine();
  if (ImGui::Button("Stop", {80, 0}))
    eng.stop();
  ImGui::SameLine();
  if (ImGui::Button("Restart", {80, 0}))
    eng.restart();

  renderEngineOptions(eng);
  return false;
}

// -----------------------------------------------------------------------
void SettingsDialog::tabBoard(AppContext &ctx) {
  GameSettings &s = ctx.settings;

  ImGui::SeparatorText("Board & Pieces");

  int theme = static_cast<int>(s.theme);
  ImGui::Text("Theme:");
  ImGui::SameLine();
  if (ImGui::RadioButton("Classic", &theme, 0))
    s.theme = BoardTheme::Classic;
  ImGui::SameLine();
  if (ImGui::RadioButton("Simple", &theme, 1))
    s.theme = BoardTheme::Simple;

  ImGui::SliderFloat("Piece scale", &s.pieceScale, 0.5f, 1.0f, "%.2f");
  ImGui::Checkbox("Flip board", &s.flipBoard);
  ImGui::Checkbox("Show coordinates", &s.showCoords);
  ImGui::Checkbox("Animate moves", &s.animateMoves);
  if (s.animateMoves)
    ImGui::SliderFloat("Anim speed (ms)", &s.animSpeedMs, 40.f, 400.f, "%.0f");
}

// -----------------------------------------------------------------------
void SettingsDialog::tabColors(AppContext &ctx) {
  GameSettings &s = ctx.settings;

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

} // namespace XiangQi
