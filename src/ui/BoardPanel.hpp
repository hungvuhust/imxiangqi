#pragma once
#include "../renderer/BoardRenderer.hpp"
#include "IPanel.hpp"

namespace XiangQi {

// -----------------------------------------------------------------------
//  BoardPanel  – dockable panel that contains ONLY the board canvas.
//
//  Responsibilities:
//    - Render the board texture + piece sprites via BoardRenderer
//    - Translate ImGui mouse clicks into GameState::onSquareClicked()
//    - Resize the board to fill the available panel area
//    - Sync BoardRenderer::Config from GameSettings every frame
//
//  Does NOT contain: buttons, move history, status text.
//  Those live in separate panels.
// -----------------------------------------------------------------------
class BoardPanel : public IPanel {
public:
  explicit BoardPanel(const TextureManager &texMgr);

  void onRender(AppContext &ctx) override;

private:
  BoardRenderer renderer_;

  // Push current GameSettings values into renderer config each frame
  void syncConfig(const GameSettings &settings);
};

} // namespace XiangQi
