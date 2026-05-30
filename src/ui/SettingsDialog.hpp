#pragma once
#include "../engine/EnginePool.hpp"
#include "AppContext.hpp"
#include <string>
#include <vector>

namespace XiangQi {

class SettingsDialog {
public:
  void open(int page = -1); // -1 = keep current page
  void render(AppContext &ctx);

private:
  bool open_          = false;
  bool confirmReset_  = false;
  int  deleteConfirm_ = -1;
  int  currentPage_   = 1; // 0=General 1=Engines 2=Appearance

  struct EngBufs {
    char path[512] = {};
    char name[128] = {};
  };
  std::vector<EngBufs> bufs_;
  bool                 bufsInit_ = false;

  void syncBuffers(const EnginePool &pool);
  void renderNav();
  void pageGeneral(AppContext &ctx);
  void pageEngines(AppContext &ctx);
  void pageAppearance(AppContext &ctx);

  // Returns true if engine was removed (caller must break loop).
  bool renderEngineCard(int idx, AppContext &ctx);
};

} // namespace XiangQi
