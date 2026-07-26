///
/// MenuBuilder — Tree construction for the main menu and all settings
///

#pragma once

#include <memory>

#include "menu_tree.h"

struct ConfigData;

namespace menu {

std::unique_ptr<IMenuNode> BuildMainMenuTree(ConfigData &cfg);

} // namespace menu
