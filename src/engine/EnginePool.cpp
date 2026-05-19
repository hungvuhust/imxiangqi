#include "EnginePool.hpp"
#include <algorithm>
#include <filesystem>
#include <sstream>

namespace XiangQi {

// -----------------------------------------------------------------------
EnginePool::Entry::Entry()
    : ctrl(std::make_unique<EngineController>()) {}

// -----------------------------------------------------------------------
int EnginePool::addEngine() {
  entries.emplace_back();
  int idx = (int)entries.size() - 1;
  ensureUniqueName(idx);
  return idx;
}

// -----------------------------------------------------------------------
void EnginePool::removeEngine(int idx) {
  if (!valid(idx))
    return;

  // Stop the engine before removing
  entries[idx].ctrl->stop();
  entries.erase(entries.begin() + idx);

  // Fix up assignments
  auto fix = [&](int &ref) {
    if (ref == idx)
      ref = -1;
    else if (ref > idx)
      --ref;
  };
  fix(redIdx);
  fix(blackIdx);
}

// -----------------------------------------------------------------------
void EnginePool::ensureUniqueName(int idx) {
  if (!valid(idx))
    return;
  Entry &e = entries[idx];

  // Base name: use settings.name if set, else derive from binary filename
  std::string base = e.settings.name;
  if (base.empty() && !e.settings.path.empty()) {
    base = std::filesystem::path(e.settings.path).stem().string();
  }
  if (base.empty())
    base = "Engine";

  // Check for duplicates among other entries
  auto isUsed = [&](const std::string &name) {
    for (int i = 0; i < (int)entries.size(); ++i) {
      if (i == idx)
        continue;
      if (entries[i].displayName == name)
        return true;
    }
    return false;
  };

  if (!isUsed(base)) {
    e.displayName = base;
    return;
  }

  // Append _1, _2, ... until unique
  for (int n = 1;; ++n) {
    std::string cand = base + "_" + std::to_string(n);
    if (!isUsed(cand)) {
      e.displayName = cand;
      return;
    }
  }
}

// -----------------------------------------------------------------------
std::string EnginePool::nameOf(int idx) const {
  if (idx == -1)
    return "Human";
  if (!valid(idx))
    return "?";
  const std::string &dn = entries[idx].displayName;
  return dn.empty() ? ("Engine " + std::to_string(idx)) : dn;
}

// -----------------------------------------------------------------------
std::vector<std::string> EnginePool::comboNames() const {
  std::vector<std::string> v;
  v.reserve(1 + entries.size());
  v.push_back("Human");
  for (const auto &e : entries)
    v.push_back(e.displayName.empty()
                    ? ("Engine " + std::to_string((int)(&e - &entries[0])))
                    : e.displayName);
  return v;
}

// -----------------------------------------------------------------------
void EnginePool::stopAll() {
  for (auto &e : entries) e.ctrl->stop();
}

} // namespace XiangQi
