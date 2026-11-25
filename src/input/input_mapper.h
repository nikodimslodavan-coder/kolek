#pragma once

#include "core/input.h"
#include "input/controls.h"
#include "input/touch_controls.h"

namespace kolek {

Input collectInput(const Controls& controls, const TouchControls& touch,
                   const Uint8* ks, const Uint8* prevKeys, bool backQueued);

} // namespace kolek
