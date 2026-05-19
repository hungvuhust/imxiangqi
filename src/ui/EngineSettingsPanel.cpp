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
                        {"All Files", "*"},
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
  ImGui::SliderInt("MultiPV", &s.multiPv, 1, 5);
  ImGui::SetItemTooltip("Number of best lines to analyse simultaneously");
  ImGui::Checkbox("Pondering", &s.ponder);
  ImGui::SetItemTooltip("Engine thinks during opponent's turn");

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

  if (!ctx.engine.engineIdName().empty())
    ImGui::Text("Engine: %s", ctx.engine.engineIdName().c_str());

  if (!ctx.engine.lastError().empty())
    ImGui::TextColored({1.f, 0.35f, 0.35f, 1.f},
                       "Error: %s",
                       ctx.engine.lastError().c_str());

  // --- Dynamic engine options -----------------------------------------
  auto &opts = ctx.engine.engineOptions();
  if (!opts.empty()) {
    ImGui::SeparatorText("Engine Options");

    bool anyDirty = false;
    for (auto &opt : opts) {
      ImGui::PushID(opt.name.c_str());

      switch (opt.type) {
      case EngineOptionType::Check: {
        bool v = opt.checkVal;
        if (ImGui::Checkbox(opt.name.c_str(), &v)) {
          opt.checkVal = v;
          opt.dirty    = true;
          anyDirty     = true;
        }
        break;
      }
      case EngineOptionType::Spin: {
        int v = opt.spinVal;
        if (ImGui::SliderInt(opt.name.c_str(), &v, opt.spinMin, opt.spinMax)) {
          opt.spinVal = v;
          opt.dirty   = true;
          anyDirty    = true;
        }
        break;
      }
      case EngineOptionType::Combo: {
        // Build items array
        std::vector<const char *> items;
        for (auto &var : opt.comboVars) items.push_back(var.c_str());
        int curIdx = 0;
        for (int i = 0; i < static_cast<int>(opt.comboVars.size()); ++i) {
          if (opt.comboVars[i] == opt.strVal) {
            curIdx = i;
            break;
          }
        }
        if (ImGui::Combo(opt.name.c_str(),
                         &curIdx,
                         items.data(),
                         static_cast<int>(items.size()))) {
          if (curIdx < static_cast<int>(opt.comboVars.size())) {
            opt.strVal = opt.comboVars[curIdx];
            opt.dirty  = true;
            anyDirty   = true;
          }
        }
        break;
      }
      case EngineOptionType::String: {
        // Use a small local buffer per option (128 chars)
        char buf[128] = {};
        std::strncpy(buf, opt.strVal.c_str(), sizeof(buf) - 1);
        if (ImGui::InputText(opt.name.c_str(),
                             buf,
                             sizeof(buf),
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
          opt.strVal = buf;
          opt.dirty  = true;
          anyDirty   = true;
        }
        break;
      }
      case EngineOptionType::Button: {
        if (ImGui::Button(opt.name.c_str())) {
          opt.dirty = true;
          anyDirty  = true;
          // Buttons send immediately
          ctx.engine.sendOption(opt);
        }
        break;
      }
      }

      ImGui::PopID();
    }

    if (anyDirty) {
      ImGui::Spacing();
      if (ImGui::Button("Apply options", {-1, 0}))
        ctx.engine.flushDirtyOptions();
    }
  }

  ImGui::End();
}

} // namespace XiangQi
