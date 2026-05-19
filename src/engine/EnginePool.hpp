#pragma once

#include "EngineController.hpp"
#include "EngineSettings.hpp"

#include <memory>
#include <string>
#include <vector>

namespace XiangQi {

// -----------------------------------------------------------------------
//  EnginePool  – owns N engine instances, maps board-sides to engines.
//
//  redIdx / blackIdx hold the index into entries[] of the engine assigned
//  to each side.  -1 means "Human" (no engine for that side).
//
//  Invariant: indices are always either -1 or a valid index into entries.
// -----------------------------------------------------------------------
struct EnginePool {
  struct Entry {
    std::string                       displayName; // user-visible name
    EngineSettings                    settings;
    std::unique_ptr<EngineController> ctrl;

    Entry();
    Entry(Entry &&)                 = default;
    Entry &operator=(Entry &&)      = default;
    // non-copyable because EngineController is non-copyable
    Entry(const Entry &)            = delete;
    Entry &operator=(const Entry &) = delete;
  };

  std::vector<Entry> entries;

  int redIdx   = -1; // index of engine assigned to Red  (-1 = Human)
  int blackIdx = -1; // index of engine assigned to Black(-1 = Human)

  // ----------------------------------------------------------------
  //  Mutators
  // ----------------------------------------------------------------

  // Add a new engine with default settings; returns its index.
  int addEngine();

  // Remove engine at idx.  Adjusts redIdx/blackIdx accordingly.
  void removeEngine(int idx);

  // Ensure displayName is unique: if empty or duplicated, auto-assign.
  void ensureUniqueName(int idx);

  // ----------------------------------------------------------------
  //  Queries
  // ----------------------------------------------------------------
  bool valid(int idx) const { return idx >= 0 && idx < (int)entries.size(); }

  // Returns nullptr when idx == -1 or out of range
  EngineController *controllerFor(int idx) {
    return valid(idx) ? entries[idx].ctrl.get() : nullptr;
  }
  EngineController *redEngine() { return controllerFor(redIdx); }
  EngineController *blackEngine() { return controllerFor(blackIdx); }
  // isRed: true → Red side, false → Black side
  EngineController *activeEngineFor(bool isRed) {
    return isRed ? redEngine() : blackEngine();
  }

  // Convenience: is this side using an engine?
  bool redIsEngine() const { return valid(redIdx); }
  bool blackIsEngine() const { return valid(blackIdx); }
  bool hasAnyEngine() const { return redIsEngine() || blackIsEngine(); }

  // Display name for an index (-1 → "Human")
  std::string nameOf(int idx) const;

  // Build list of names for combo boxes: ["Human", engine0, engine1, ...]
  // Returns indices such that comboIndex==0 → Human (-1),
  //                           comboIndex==k → entries[k-1]
  std::vector<std::string> comboNames() const;
  // Convert combo index ↔ entry index
  int comboToIdx(int comboIdx) const { return comboIdx - 1; }
  int idxToCombo(int idx) const { return idx + 1; }

  // Stop all running engines
  void stopAll();
};

} // namespace XiangQi
