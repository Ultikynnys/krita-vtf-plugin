// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <functional>

namespace VtfGuiDispatch {

// Runs action on QApplication's GUI thread. When called from another thread,
// this function blocks only until the GUI-thread action returns.
bool runBlocking(const std::function<void()> &action);

} // namespace VtfGuiDispatch
