#pragma once
#include "AppContext.hpp"
#include <string>

namespace XiangQi {

// -----------------------------------------------------------------------
//  IPanel  – base interface for every dockable ImGui panel.
//
//  Contract:
//    - Each panel owns its ImGui window (calls Begin/End itself).
//    - Panels communicate ONLY through AppContext.
//    - Panels must not store raw pointers to other panels.
//
//  Lifecycle (called by Application every frame):
//    1. onUpdate(ctx)  – logic update (non-rendering, e.g. animation tick)
//    2. onRender(ctx)  – ImGui Begin … draw … End
//
//  Visibility:
//    - isOpen() / setOpen() let panels be hidden via View menu.
//    - Panels should pass &open_ to ImGui::Begin() to get the X button.
// -----------------------------------------------------------------------
class IPanel {
public:
  explicit IPanel(std::string title)
      : title_(std::move(title)) {}
  virtual ~IPanel() = default;

  // Non-copyable
  IPanel(const IPanel &)            = delete;
  IPanel &operator=(const IPanel &) = delete;

  // Per-frame logic update (optional override)
  virtual void onUpdate(AppContext & /*ctx*/) {}

  // Per-frame render – MUST call ImGui::Begin/End internally
  virtual void onRender(AppContext &ctx) = 0;

  const std::string &title() const { return title_; }
  bool               isOpen() const { return open_; }
  void               setOpen(bool v) { open_ = v; }

protected:
  std::string title_;
  bool        open_ = true;
};

} // namespace XiangQi
