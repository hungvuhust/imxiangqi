#include "SettingsPanel.hpp"

#include <portable-file-dialogs.h>

#include <cstring>
#include <filesystem>
#include <imgui.h>

namespace {

std::string defaultPickerPath(const std::string &enginePath) {
  if (enginePath.empty())
    return ".";

  std::filesystem::path p(enginePath);
  if (p.has_parent_path())
    return p.parent_path().string();

  return enginePath;
}

} // namespace

namespace XiangQi {

SettingsPanel::SettingsPanel()
    : IPanel("Settings") {}

void SettingsPanel::syncEngineBuffers(const GameSettings &s) {
  if (engineBufInit_)
    return;

  std::strncpy(pathBuf_, s.enginePath.c_str(), sizeof(pathBuf_) - 1);
  pathBuf_[sizeof(pathBuf_) - 1] = '\0';

  std::strncpy(nameBuf_, s.engineName.c_str(), sizeof(nameBuf_) - 1);
  nameBuf_[sizeof(nameBuf_) - 1] = '\0';

  engineBufInit_ = true;
}

bool SettingsPanel::browseEnginePath(GameSettings &s) {
  pfd::open_file picker("Select engine binary",
                        defaultPickerPath(s.enginePath),
                        {"Executable files",
                         "*.exe *.bin *.out *.xq *.uci *.ucci",
                         "All Files",
                         "*"},
                        pfd::opt::none);

  auto files = picker.result();
  if (files.empty())
    return false;

  std::strncpy(pathBuf_, files[0].c_str(), sizeof(pathBuf_) - 1);
  pathBuf_[sizeof(pathBuf_) - 1] = '\0';
  s.enginePath                   = pathBuf_;
  return true;
}

void SettingsPanel::renderVisualSection(GameSettings &s) {
  ImGui::SeparatorText("Board & Pieces");

  int theme = static_cast<int>(s.theme);
  ImGui::Text("Theme:");
  ImGui::SameLine();
  if (ImGui::RadioButton("Classic##th", &theme, 0))
    s.theme = BoardTheme::Classic;
  ImGui::SameLine();
  if (ImGui::RadioButton("Simple##th", &theme, 1))
    s.theme = BoardTheme::Simple;

  ImGui::SliderFloat("Piece scale", &s.pieceScale, 0.5f, 1.0f, "%.2f");
  ImGui::Checkbox("Flip board", &s.flipBoard);
  ImGui::Checkbox("Show coordinates", &s.showCoords);
  ImGui::Checkbox("Animate moves", &s.animateMoves);
  if (s.animateMoves)
    ImGui::SliderFloat("Anim speed (ms)", &s.animSpeedMs, 40.f, 400.f, "%.0f");
}

void SettingsPanel::renderHighlightSection(GameSettings &s) {
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

void SettingsPanel::renderEngineSection(AppContext &ctx, GameSettings &s) {
  ImGui::SeparatorText("Players");

  const char *modes[] = {"Human", "Engine"};

  int red = static_cast<int>(s.redPlayer);
  if (ImGui::Combo("Red (South)##pl", &red, modes, 2))
    s.redPlayer = static_cast<PlayerMode>(red);

  int blk = static_cast<int>(s.blackPlayer);
  if (ImGui::Combo("Black (North)##pl", &blk, modes, 2))
    s.blackPlayer = static_cast<PlayerMode>(blk);

  if (!s.hasEngine())
    return;

  syncEngineBuffers(s);

  ImGui::SeparatorText("Engine");

  if (ImGui::InputText("Engine path", pathBuf_, sizeof(pathBuf_)))
    s.enginePath = pathBuf_;

  ImGui::SameLine();
  if (ImGui::Button("Browse...", {100, 0}))
    browseEnginePath(s);

  if (ImGui::InputText("Engine name", nameBuf_, sizeof(nameBuf_)))
    s.engineName = nameBuf_;

  ImGui::SliderInt("Depth", &s.engineDepth, 1, 30);
  ImGui::SliderInt("Time (ms)", &s.engineTimeMs, 100, 10000);
  ImGui::Checkbox("Pondering", &s.enginePonder);

  ImGui::SeparatorText("Engine runtime");

  if (ImGui::Button("Start", {90, 0}))
    ctx.engine.start();
  ImGui::SameLine();
  if (ImGui::Button("Stop", {90, 0}))
    ctx.engine.stop();
  ImGui::SameLine();
  if (ImGui::Button("Restart", {90, 0}))
    ctx.engine.restart();

  ImGui::Text("State: %s", toString(ctx.engine.state()));
  ImGui::Text("Protocol: %s", toString(ctx.engine.protocol()));
  ImGui::Text("Running: %s", ctx.engine.isRunning() ? "Yes" : "No");
  ImGui::Text("Thinking: %s", ctx.engine.isThinking() ? "Yes" : "No");

  if (!ctx.engine.lastError().empty())
    ImGui::TextColored({1.f, 0.35f, 0.35f, 1.f},
                       "Error: %s",
                       ctx.engine.lastError().c_str());
}

void SettingsPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  GameSettings &s = ctx.settings;

  renderVisualSection(s);
  renderHighlightSection(s);
  renderEngineSection(ctx, s);

  ImGui::Separator();

  if (!confirmReset_) {
    if (ImGui::Button("Reset to defaults", {-1, 0}))
      confirmReset_ = true;
  } else {
    ImGui::TextColored({1, 0.4f, 0.2f, 1}, "Reset all settings?");
    if (ImGui::Button("Yes, reset", {100, 0})) {
      s.reset();
      engineBufInit_ = false;
      confirmReset_  = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", {80, 0}))
      confirmReset_ = false;
  }

  ImGui::End();
}

} // namespace XiangQi
