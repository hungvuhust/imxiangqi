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

} // namespace

namespace XiangQi {

static constexpr const char *POPUP_ID = "Settings##modal";

void SettingsDialog::open() {
  pendingOpen_ = true;
  bufInit_     = false; // re-sync buffers với settings hiện tại
}

void SettingsDialog::syncBuffers(const GameSettings &s) {
  if (bufInit_)
    return;
  std::strncpy(pathBufRed_, s.engineRed.path.c_str(), sizeof(pathBufRed_) - 1);
  std::strncpy(nameBufRed_, s.engineRed.name.c_str(), sizeof(nameBufRed_) - 1);
  std::strncpy(pathBufBlack_,
               s.engineBlack.path.c_str(),
               sizeof(pathBufBlack_) - 1);
  std::strncpy(nameBufBlack_,
               s.engineBlack.name.c_str(),
               sizeof(nameBufBlack_) - 1);
  bufInit_ = true;
}

bool SettingsDialog::browseEnginePath(EngineSettings &s,
                                      char           *pathBuf,
                                      size_t          pathBufSize) {
  pfd::open_file picker("Select engine binary",
                        defaultPickerPath(s.path),
                        {"All Files", "*"},
                        pfd::opt::none);
  auto           files = picker.result();
  if (files.empty())
    return false;
  std::strncpy(pathBuf, files[0].c_str(), pathBufSize - 1);
  pathBuf[pathBufSize - 1] = '\0';
  s.path                   = pathBuf;
  return true;
}

// ---------------------------------------------------------------------------
void SettingsDialog::render(AppContext &ctx) {
  // OpenPopup phải gọi cùng level với BeginPopupModal (không nằm trong Begin)
  if (pendingOpen_) {
    ImGui::OpenPopup(POPUP_ID);
    pendingOpen_ = false;
  }

  // Center popup lần đầu
  ImVec2 center = ImGui::GetMainViewport()->GetCenter();
  ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, {0.5f, 0.5f});
  ImGui::SetNextWindowSize({620, 520}, ImGuiCond_Appearing);

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

  // Footer: Reset + Close
  if (!confirmReset_) {
    if (ImGui::Button("Reset to defaults"))
      confirmReset_ = true;
  } else {
    ImGui::TextColored({1.f, 0.4f, 0.2f, 1.f}, "Reset all settings?");
    ImGui::SameLine();
    if (ImGui::Button("Yes")) {
      ctx.settings.reset();
      bufInit_      = false;
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

// ---------------------------------------------------------------------------
// Helper: render engine options for one engine
static void renderEngineOptions(EngineController &eng) {
  auto &opts = eng.engineOptions();
  if (opts.empty())
    return;
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
                       (int)items.size())) {
        if (curIdx < (int)opt.comboVars.size()) {
          opt.strVal = opt.comboVars[curIdx];
          opt.dirty = anyDirty = true;
        }
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
}

void SettingsDialog::renderEngineSettings(const char       *label,
                                          EngineSettings   &s,
                                          EngineController &eng,
                                          char             *pathBuf,
                                          size_t            pathBufSize,
                                          char             *nameBuf,
                                          size_t            nameBufSize) {
  ImGui::SeparatorText(label);

  // Unique ID suffix per side
  auto id = [&](const char *base) { return std::string(base) + "##" + label; };

  ImGui::SetNextItemWidth(-110);
  if (ImGui::InputText(id("Engine path").c_str(), pathBuf, pathBufSize))
    s.path = pathBuf;
  ImGui::SameLine();
  if (ImGui::Button(id("Browse...").c_str(), {100, 0}))
    browseEnginePath(s, pathBuf, pathBufSize);

  if (ImGui::InputText(id("Engine name").c_str(), nameBuf, nameBufSize))
    s.name = nameBuf;

  ImGui::SliderInt(id("Depth").c_str(), &s.depth, 1, 30);
  ImGui::SliderInt(id("Time (ms)").c_str(), &s.timeMs, 100, 10000);
  ImGui::SliderInt(id("MultiPV").c_str(), &s.multiPv, 1, 5);
  ImGui::SetItemTooltip("Number of best lines to analyse simultaneously");
  ImGui::Checkbox(id("Pondering").c_str(), &s.ponder);
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

  if (ImGui::Button(id("Start").c_str(), {80, 0}))
    eng.start();
  ImGui::SameLine();
  if (ImGui::Button(id("Stop").c_str(), {80, 0}))
    eng.stop();
  ImGui::SameLine();
  if (ImGui::Button(id("Restart").c_str(), {80, 0}))
    eng.restart();

  renderEngineOptions(eng);
}

// ---------------------------------------------------------------------------
void SettingsDialog::tabGeneral(AppContext &ctx) {
  GameSettings &gs = ctx.settings;

  ImGui::SeparatorText("Players");

  const char *modes[] = {"Human", "Engine"};
  int         red     = static_cast<int>(gs.redPlayer);
  if (ImGui::Combo("Red (South)", &red, modes, 2))
    gs.redPlayer = static_cast<PlayerMode>(red);

  int blk = static_cast<int>(gs.blackPlayer);
  if (ImGui::Combo("Black (North)", &blk, modes, 2))
    gs.blackPlayer = static_cast<PlayerMode>(blk);
}

// ---------------------------------------------------------------------------
void SettingsDialog::tabEngine(AppContext &ctx) {
  GameSettings &gs = ctx.settings;

  syncBuffers(gs);

  renderEngineSettings("Red Engine",
                       gs.engineRed,
                       ctx.engineRed,
                       pathBufRed_,
                       sizeof(pathBufRed_),
                       nameBufRed_,
                       sizeof(nameBufRed_));
  ImGui::Spacing();
  renderEngineSettings("Black Engine",
                       gs.engineBlack,
                       ctx.engineBlack,
                       pathBufBlack_,
                       sizeof(pathBufBlack_),
                       nameBufBlack_,
                       sizeof(nameBufBlack_));
}

// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
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
