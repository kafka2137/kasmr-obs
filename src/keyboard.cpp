#include "keyboard.hpp"
#include <algorithm>

namespace kasmr {
bool KeyState::accept(unsigned key, bool release)
{
    if (key >= down_.size() || down_[key] == !release) return false;
    down_[key] = !release;
    return true;
}

bool layout_accepts(Layout layout, unsigned scan, bool extended)
{
    if (scan == 0 || scan > 0x7f) return false;
    if (layout == Layout::Full) return true;
    const bool numpad = (!extended && scan >= 0x47 && scan <= 0x53) || (!extended && (scan == 0x37 || scan == 0x45)) || (extended && (scan == 0x1c || scan == 0x35));
    if (numpad) return false;
    const bool function = (scan >= 0x3b && scan <= 0x44) || scan == 0x57 || scan == 0x58;
    if ((layout == Layout::Compact65 || layout == Layout::Hhkb) && function) return false;
    if (layout == Layout::Hhkb && extended && scan >= 0x47 && scan <= 0x53) return false;
    if (layout == Layout::Compact75 && extended && (scan == 0x52 || scan == 0x47 || scan == 0x4f)) return false;
    if (layout == Layout::Compact65 && extended && (scan == 0x52 || scan == 0x47)) return false;
    return true;
}

unsigned key_sound(unsigned scan, bool release)
{
    unsigned special = scan == 0x39 ? 0 : scan == 0x1c ? 1 : scan == 0x0e ? 2 : 3;
    if (release) return 8 + special;
    if (special < 3) return special;
    if (scan >= 0x02 && scan <= 0x0d) return 3;
    if (scan >= 0x10 && scan <= 0x1b) return 4;
    if (scan >= 0x1e && scan <= 0x28) return 5;
    if (scan >= 0x2c && scan <= 0x35) return 6;
    return 7;
}

float key_pan(unsigned scan, bool extended)
{
    if (extended && scan >= 0x47) return 0.85f;
    if (scan == 0x39) return 0;
    if (scan >= 0x02 && scan <= 0x0e) return (static_cast<float>(scan - 2) / 12 - 0.5f) * 1.6f;
    if (scan >= 0x10 && scan <= 0x1c) return (static_cast<float>(scan - 0x10) / 12 - 0.5f) * 1.6f;
    if (scan >= 0x1e && scan <= 0x28) return (static_cast<float>(scan - 0x1e) / 10 - 0.5f) * 1.6f;
    if (scan >= 0x2c && scan <= 0x35) return (static_cast<float>(scan - 0x2c) / 9 - 0.5f) * 1.6f;
    return 0;
}
}
