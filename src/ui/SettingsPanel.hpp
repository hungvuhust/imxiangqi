#pragma once
#include "IPanel.hpp"

namespace XiangQi {

// -----------------------------------------------------------------------
//  SettingsPanel  – visual & engine configuration, edits GameSettings
// -----------------------------------------------------------------------
class SettingsPanel : public IPanel {
public:
  SettingsPanel();
  void onRender(AppContext &ctx) override;

private:
  // Transient UI state
  bool confirmReset_ = false;

  void renderVisualSection(GameSettings &s);
  void renderHighlightSection(GameSettings &s);
  void renderEngineSection(GameSettings &s);
};

} // namespace XiangQi
