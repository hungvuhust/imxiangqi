#include "EngineSettingsPanel.hpp"

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

EngineSettingsPanel::EngineSettingsPanel()
    : IPanel("Engine Settings") {}

void EngineSettingsPanel::syncBuffers(const EngineSettings &s) {
  if (bufInit_)
    return;
  std::strncpy(pathBuf_, s.path.c_str(), sizeof(pathBuf_) - 1);
  pathBuf_[sizeof(pathBuf_) - 1] = '\0';
  std::strncpy(nameBuf_, s.name.c_str(), sizeof(nameBuf_) - 1);
  nameBuf_[sizeof(nameBuf_) - 1] = '\0';
  bufInit_                       = true;
}

bool EngineSettingsPanel::browseEnginePath(EngineSettings &s) {
  pfd::open_file picker("Select engine binary",
                        defaultPickerPath(s.path),
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
  s.path                         = pathBuf_;
  return true;
}

void EngineSettingsPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  GameSettings   &gs = ctx.settings;
  EngineSettings &s  = gs.engine;

  ImGui::SeparatorText("Players");

  const char *modes[] = {"Human", "Engine"};

  int red = static_cast<int>(gs.redPlayer);
  if (ImGui::Combo("Red (South)##pl", &red, modes, 2))
    gs.redPlayer = static_cast<PlayerMode>(red);

  int blk = static_cast<int>(gs.blackPlayer);
  if (ImGui::Combo("Black (North)##pl", &blk, modes, 2))
    gs.blackPlayer = static_cast<PlayerMode>(blk);

  ImGui::SeparatorText("Engine Configuration");

  syncBuffers(s);

  if (ImGui::InputText("Engine path", pathBuf_, sizeof(pathBuf_)))
    s.path = pathBuf_;

  ImGui::SameLine();
  if (ImGui::Button("Browse...", {100, 0}))
    browseEnginePath(s);

  if (ImGui::InputText("Engine name", nameBuf_, sizeof(nameBuf_)))
    s.name = nameBuf_;

  ImGui::SliderInt("Depth", &s.depth, 1, 30);
  ImGui::SliderInt("Time (ms)", &s.timeMs, 100, 10000);
  ImGui::Checkbox("Pondering", &s.ponder);

  ImGui::SeparatorText("Runtime Control");

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

  if (!ctx.engine.lastError().empty())
    ImGui::TextColored({1.f, 0.35f, 0.35f, 1.f},
                       "Error: %s",
                       ctx.engine.lastError().c_str());

  ImGui::End();
}

} // namespace XiangQi
