#pragma once

#include "IPanel.hpp"

#include <string>

namespace XiangQi {

class SettingsPanel : public IPanel {
public:
  SettingsPanel();
  void onRender(AppContext &ctx) override;

private:
  bool confirmReset_ = false;

  char pathBuf_[512]  = {};
  char nameBuf_[128]  = {};
  bool engineBufInit_ = false;

  void syncEngineBuffers(const GameSettings &s);
  bool browseEnginePath(GameSettings &s);

  void renderVisualSection(GameSettings &s);
  void renderHighlightSection(GameSettings &s);
  void renderEngineSection(AppContext &ctx, GameSettings &s);
};

} // namespace XiangQi
