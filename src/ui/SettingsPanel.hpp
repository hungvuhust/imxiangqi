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

  char pathBuf_[512]  = {};
  char nameBuf_[128]  = {};
  bool engineBufInit_ = false;

  void syncEngineBuffers(const GameSettings &s);

  void renderVisualSection(GameSettings &s);
  void renderHighlightSection(GameSettings &s);
  void renderEngineSection(AppContext &ctx, GameSettings &s);
};

} // namespace XiangQi
