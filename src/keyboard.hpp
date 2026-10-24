#pragma once
#include <array>
#include <cstdint>

namespace kasmr {
enum class Layout { Full, Tkl, Compact75, Compact65, Hhkb };
enum class Case { Original, Plastic, Aluminum, Wood };
bool layout_accepts(Layout layout, unsigned scan, bool extended);
unsigned key_sound(unsigned scan, bool release);
float key_pan(unsigned scan, bool extended);
class KeyState {
public:
    bool accept(unsigned key, bool release);
private:
    std::array<bool, 512> down_{};
};
}
