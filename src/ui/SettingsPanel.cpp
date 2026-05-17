#include "SettingsPanel.hpp"

#include <portable-file-dialogs.h>

#include <cstring>
#include <filesystem>
#include <imgui.h>

namespace XiangQi {

SettingsPanel::SettingsPanel()
    : IPanel("Settings") {}

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

  ImGui::Separator();

  if (!confirmReset_) {
    if (ImGui::Button("Reset to defaults", {-1, 0}))
      confirmReset_ = true;
  } else {
    ImGui::TextColored({1, 0.4f, 0.2f, 1}, "Reset all settings?");
    if (ImGui::Button("Yes, reset", {100, 0})) {
      s.reset();
      confirmReset_ = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", {80, 0}))
      confirmReset_ = false;
  }

  ImGui::End();
}

} // namespace XiangQi
