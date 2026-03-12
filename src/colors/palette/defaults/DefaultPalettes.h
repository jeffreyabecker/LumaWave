#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

#include "core/Compat.h"
#include "colors/Colors.h"
#include "colors/palette/Generators.h"

namespace lw::colors::palettes
{
template <typename TColor, typename = std::enable_if_t<ColorType<TColor>>> class StaticPalette : public IPalette<TColor>
{
  public:
    using StopsView = typename IPalette<TColor>::StopsView;

    StaticPalette() = default;

    // Caller convention: stops are expected in non-decreasing index order.
    // Duplicate indexes are allowed and create zero-width transitions.
    explicit StaticPalette(StopsView stops) : _stops(stops) {}

    template <size_t N>
    explicit StaticPalette(const std::array<PaletteStop<TColor>, N>& stops) : _stops(stops.data(), stops.size())
    {
    }

    StopsView stops() const override { return _stops; }

    void update(uint32_t = 0) override {}

  private:
    StopsView _stops{};
};
} // namespace lw::colors::palettes

namespace lw::palettes
{
template <typename TColor = colors::DefaultColorType> struct NamedPalette
{
    const char* name;
    colors::palettes::StaticPalette<TColor> (*create)();
};

namespace default_palette_data
{
using StopData = std::pair<size_t, uint32_t>;

template <size_t N> using StopDataArray = std::array<StopData, N>;

inline constexpr StopDataArray<16> Party = {{
    {0, 0x5500AB},
    {17, 0x84007C},
    {34, 0xB5004B},
    {51, 0xE5001B},
    {68, 0xE81700},
    {85, 0xB84700},
    {102, 0xAB7700},
    {119, 0xABAB00},
    {136, 0xAB5500},
    {153, 0xDD2200},
    {170, 0xF2000E},
    {187, 0xC2003E},
    {204, 0x8F0071},
    {221, 0x5F00A1},
    {238, 0x2F00D0},
    {255, 0x0007F9},
}};

inline constexpr StopDataArray<16> Cloud = {{
    {0, 0x0000FF},
    {17, 0x00008B},
    {34, 0x00008B},
    {51, 0x00008B},
    {68, 0x00008B},
    {85, 0x00008B},
    {102, 0x00008B},
    {119, 0x00008B},
    {136, 0x0000FF},
    {153, 0x00008B},
    {170, 0x87CEEB},
    {187, 0x87CEEB},
    {204, 0xADD8E6},
    {221, 0xFFFFFF},
    {238, 0xADD8E6},
    {255, 0x87CEEB},
}};

inline constexpr StopDataArray<16> Lava = {{
    {0, 0x000000},
    {17, 0x800000},
    {34, 0x000000},
    {51, 0x800000},
    {68, 0x8B0000},
    {85, 0x8B0000},
    {102, 0x800000},
    {119, 0x8B0000},
    {136, 0x8B0000},
    {153, 0x8B0000},
    {170, 0xFF0000},
    {187, 0xFFA500},
    {204, 0xFFFFFF},
    {221, 0xFFA500},
    {238, 0xFF0000},
    {255, 0x8B0000},
}};

inline constexpr StopDataArray<16> Ocean = {{
    {0, 0x191970},
    {17, 0x00008B},
    {34, 0x191970},
    {51, 0x000080},
    {68, 0x00008B},
    {85, 0x0000CD},
    {102, 0x2E8B57},
    {119, 0x008080},
    {136, 0x5F9EA0},
    {153, 0x0000FF},
    {170, 0x008B8B},
    {187, 0x6495ED},
    {204, 0x7FFFD4},
    {221, 0x2E8B57},
    {238, 0x00FFFF},
    {255, 0x87CEFA},
}};

inline constexpr StopDataArray<16> Forest = {{
    {0, 0x006400},
    {17, 0x006400},
    {34, 0x556B2F},
    {51, 0x006400},
    {68, 0x008000},
    {85, 0x228B22},
    {102, 0x6B8E23},
    {119, 0x008000},
    {136, 0x2E8B57},
    {153, 0x66CDAA},
    {170, 0x32CD32},
    {187, 0x9ACD32},
    {204, 0x90EE90},
    {221, 0x7CFC00},
    {238, 0x66CDAA},
    {255, 0x228B22},
}};

inline constexpr StopDataArray<16> Rainbow = {{
    {0, 0xFF0000},
    {17, 0xD52A00},
    {34, 0xAB5500},
    {51, 0xAB7F00},
    {68, 0xABAB00},
    {85, 0x56D500},
    {102, 0x00FF00},
    {119, 0x00D52A},
    {136, 0x00AB55},
    {153, 0x0056AA},
    {170, 0x0000FF},
    {187, 0x2A00D5},
    {204, 0x5500AB},
    {221, 0x7F0081},
    {238, 0xAB0055},
    {255, 0xD5002B},
}};

inline constexpr StopDataArray<16> RainbowBands = {{
    {0, 0xFF0000},
    {17, 0x000000},
    {34, 0xAB5500},
    {51, 0x000000},
    {68, 0xABAB00},
    {85, 0x000000},
    {102, 0x00FF00},
    {119, 0x000000},
    {136, 0x00AB55},
    {153, 0x000000},
    {170, 0x0000FF},
    {187, 0x000000},
    {204, 0x5500AB},
    {221, 0x000000},
    {238, 0xAB0055},
    {255, 0x000000},
}};

inline constexpr StopDataArray<7> Sunset = {{
    {0, 0x780000},
    {22, 0xB31600},
    {51, 0xFF6800},
    {85, 0xA71612},
    {135, 0x640067},
    {198, 0x100082},
    {255, 0x0000A0},
}};

inline constexpr StopDataArray<5> Rivendell = {{
    {0, 0x010E05},
    {101, 0x10240E},
    {165, 0x38441E},
    {242, 0x969C63},
    {255, 0x969C63},
}};

inline constexpr StopDataArray<4> Breeze = {{
    {0, 0x010607},
    {89, 0x01636F},
    {153, 0x90D1FF},
    {255, 0x004952},
}};

inline constexpr StopDataArray<9> RedBlue = {{
    {0, 0x040146},
    {31, 0x37011E},
    {63, 0xFF0407},
    {95, 0x3B021D},
    {127, 0x0B0332},
    {159, 0x27083C},
    {191, 0x701328},
    {223, 0x4E0B27},
    {255, 0x1D083B},
}};

inline constexpr StopDataArray<2> Yellowout = {{
    {0, 0xBC8701},
    {255, 0x2E0701},
}};

inline constexpr StopDataArray<5> Analogous = {{
    {0, 0x0300FF},
    {63, 0x1700FF},
    {127, 0x4300FF},
    {191, 0x8E002D},
    {255, 0xFF0000},
}};

inline constexpr StopDataArray<5> Splash = {{
    {0, 0x7E0BFF},
    {127, 0xC50116},
    {175, 0xD29DAC},
    {221, 0x9D0370},
    {255, 0x9D0370},
}};

inline constexpr StopDataArray<11> Pastel = {{
    {0, 0x0A3E7B},
    {36, 0x388267},
    {87, 0x99E155},
    {100, 0xC7D944},
    {107, 0xFFCF36},
    {115, 0xF79839},
    {120, 0xEF6B3D},
    {128, 0xF79839},
    {180, 0xFFCF36},
    {223, 0xFFE330},
    {255, 0xFFF82A},
}};

inline constexpr StopDataArray<8> Sunset2 = {{
    {0, 0x6E310B},
    {29, 0x37220A},
    {68, 0x161609},
    {68, 0xEF7C08},
    {97, 0xDC9C1B},
    {124, 0xCBC13D},
    {178, 0x213538},
    {255, 0x000134},
}};

inline constexpr StopDataArray<15> Beach = {{
    {0, 0xFFFCD6},
    {12, 0xFFFCD6},
    {22, 0xFFFCD6},
    {26, 0xBEBF73},
    {28, 0x898D34},
    {28, 0x70FFCD},
    {50, 0x33F6D6},
    {71, 0x11EBE2},
    {93, 0x02C1C7},
    {120, 0x009CAE},
    {133, 0x016573},
    {136, 0x013B47},
    {136, 0x0783AA},
    {208, 0x015A97},
    {255, 0x003885},
}};

inline constexpr StopDataArray<8> Vintage = {{
    {0, 0x040101},
    {51, 0x100001},
    {76, 0x616803},
    {101, 0xFF8313},
    {127, 0x430904},
    {153, 0x100001},
    {229, 0x040101},
    {255, 0x040101},
}};

inline constexpr StopDataArray<12> Departure = {{
    {0, 0x080300},
    {42, 0x170700},
    {63, 0x4B2606},
    {84, 0xA96326},
    {106, 0xD5A977},
    {116, 0xFFFFFF},
    {138, 0x87FF8A},
    {148, 0x16FF18},
    {170, 0x00FF00},
    {191, 0x008800},
    {212, 0x003700},
    {255, 0x003700},
}};

inline constexpr StopDataArray<9> Landscape = {{
    {0, 0x000000},
    {37, 0x021901},
    {76, 0x0F7305},
    {127, 0x4FD501},
    {128, 0x7ED32F},
    {130, 0xBCD1F7},
    {153, 0x90B6CD},
    {204, 0x3B75FA},
    {255, 0x0125C0},
}};

inline constexpr StopDataArray<6> Beech = {{
    {0, 0x010500},
    {19, 0x201701},
    {38, 0xA13701},
    {63, 0xE59001},
    {66, 0x278E4A},
    {255, 0x010401},
}};

inline constexpr StopDataArray<7> Sherbet = {{
    {0, 0xFF2104},
    {43, 0xFF4419},
    {86, 0xFF0719},
    {127, 0xFF5267},
    {170, 0xFFFFF2},
    {209, 0x2AFF16},
    {255, 0x57FF41},
}};

inline constexpr StopDataArray<6> Hult = {{
    {0, 0xF7B0F7},
    {48, 0xFF88FF},
    {89, 0xDC1DE2},
    {160, 0x0752B2},
    {216, 0x017C6D},
    {255, 0x017C6D},
}};

inline constexpr StopDataArray<8> Hult64 = {{
    {0, 0x017C6D},
    {66, 0x015D4F},
    {104, 0x344101},
    {130, 0x737F01},
    {150, 0x344101},
    {201, 0x015648},
    {239, 0x00372D},
    {255, 0x00372D},
}};

inline constexpr StopDataArray<7> Drywet = {{
    {0, 0x2F1E02},
    {42, 0xD59318},
    {84, 0x67DB34},
    {127, 0x03DBCF},
    {170, 0x0130D6},
    {212, 0x01016F},
    {255, 0x010721},
}};

inline constexpr StopDataArray<4> Jul = {{
    {0, 0xC20101},
    {94, 0x011D12},
    {132, 0x39831C},
    {255, 0x710101},
}};

inline constexpr StopDataArray<5> Grintage = {{
    {0, 0x020101},
    {53, 0x120100},
    {104, 0x451D01},
    {153, 0xA7870A},
    {255, 0x2E3804},
}};

inline constexpr StopDataArray<6> Rewhi = {{
    {0, 0x715B93},
    {72, 0x9D584E},
    {89, 0xD05521},
    {107, 0xFF1D0B},
    {141, 0x891F27},
    {255, 0x3B2159},
}};

inline constexpr StopDataArray<5> Tertiary = {{
    {0, 0x0001FF},
    {63, 0x03442D},
    {127, 0x17FF00},
    {191, 0x644401},
    {255, 0xFF0104},
}};

inline constexpr StopDataArray<13> Fire = {{
    {0, 0x000000},
    {46, 0x120000},
    {96, 0x710000},
    {108, 0x8E0301},
    {119, 0xAF1101},
    {146, 0xD52C02},
    {174, 0xFF5204},
    {188, 0xFF7304},
    {202, 0xFF9C04},
    {218, 0xFFCB04},
    {234, 0xFFFF04},
    {244, 0xFFFF47},
    {255, 0xFFFFFF},
}};

inline constexpr StopDataArray<7> Icefire = {{
    {0, 0x000000},
    {59, 0x00092D},
    {119, 0x0026FF},
    {149, 0x0364FF},
    {180, 0x17C7FF},
    {217, 0x64EBFF},
    {255, 0xFFFFFF},
}};

inline constexpr StopDataArray<11> Cyane = {{
    {0, 0x0A5505},
    {25, 0x1D6D12},
    {60, 0x3B8A2A},
    {93, 0x536334},
    {106, 0x6E4240},
    {109, 0x7B3141},
    {113, 0x8B2342},
    {116, 0xC07562},
    {124, 0xFFFF89},
    {168, 0x64B49B},
    {255, 0x1679AE},
}};

inline constexpr StopDataArray<11> LightPink = {{
    {0, 0x130227},
    {25, 0x1A042D},
    {51, 0x210634},
    {76, 0x443E7D},
    {102, 0x76BBF0},
    {109, 0xA3D7F7},
    {114, 0xD9F4FF},
    {122, 0x9F95DD},
    {149, 0x714EBC},
    {183, 0x80399B},
    {255, 0x92287B},
}};

inline constexpr StopDataArray<13> Autumn = {{
    {0, 0x1A0101},
    {51, 0x430401},
    {84, 0x760E01},
    {104, 0x899834},
    {112, 0x714101},
    {122, 0x85953B},
    {124, 0x899834},
    {135, 0x714101},
    {142, 0x8B9A2E},
    {163, 0x710D01},
    {204, 0x370301},
    {249, 0x110101},
    {255, 0x110101},
}};

inline constexpr StopDataArray<7> Magenta = {{
    {0, 0x000000},
    {42, 0x00002D},
    {84, 0x0000FF},
    {127, 0x2A00FF},
    {170, 0xFF00FF},
    {212, 0xFF37FF},
    {255, 0xFFFFFF},
}};

inline constexpr StopDataArray<5> Magred = {{
    {0, 0x000000},
    {63, 0x2A002D},
    {127, 0xFF00FF},
    {191, 0xFF002D},
    {255, 0xFF0000},
}};

inline constexpr StopDataArray<7> Yelmag = {{
    {0, 0x000000},
    {42, 0x2A0000},
    {84, 0xFF0000},
    {127, 0xFF002D},
    {170, 0xFF00FF},
    {212, 0xFF372D},
    {255, 0xFFFF00},
}};

inline constexpr StopDataArray<5> Yelblu = {{
    {0, 0x0000FF},
    {63, 0x0037FF},
    {127, 0x00FFFF},
    {191, 0x2AFF2D},
    {255, 0xFFFF00},
}};

inline constexpr StopDataArray<4> OrangeTeal = {{
    {0, 0x00965C},
    {55, 0x00965C},
    {200, 0xFF4800},
    {255, 0xFF4800},
}};

inline constexpr StopDataArray<17> Tiamat = {{
    {0, 0x01020E},
    {33, 0x020523},
    {47, 0x3D7E64},
    {13, 0x875C58},
    {242, 0xF7782B},
    {255, 0xC187FF},
    {253, 0x8CF707},
    {249, 0xFC45FD},
    {160, 0xC111D0},
    {231, 0x60EDB4},
    {39, 0xFF9A82},
    {77, 0xD5C804},
    {213, 0xEC397A},
    {248, 0xDC27FC},
    {135, 0xB1FEFF},
    {240, 0xC1D5FD},
    {203, 0xEFFDFF},
}};

inline constexpr StopDataArray<17> AprilNight = {{
    {0, 0x01052D},
    {10, 0x01052D},
    {25, 0x05A9AF},
    {40, 0x01052D},
    {61, 0x01052D},
    {76, 0x2DAF1F},
    {91, 0x01052D},
    {112, 0x01052D},
    {127, 0xF99605},
    {143, 0x01052D},
    {162, 0x01052D},
    {178, 0xFF5C00},
    {193, 0x01052D},
    {214, 0x01052D},
    {229, 0xDF2D48},
    {244, 0x01052D},
    {255, 0x01052D},
}};

inline constexpr StopDataArray<9> Orangery = {{
    {0, 0xFF5F17},
    {30, 0xFF5200},
    {60, 0xDF0D08},
    {90, 0x902C02},
    {120, 0xFF6E11},
    {150, 0xFF4500},
    {180, 0x9E0D0B},
    {210, 0xF15211},
    {255, 0xD52504},
}};

inline constexpr StopDataArray<8> C9 = {{
    {0, 0xB80400},
    {60, 0xB80400},
    {65, 0x902C02},
    {125, 0x902C02},
    {130, 0x046002},
    {190, 0x046002},
    {195, 0x070758},
    {255, 0x070758},
}};

inline constexpr StopDataArray<5> Sakura = {{
    {0, 0xC4130A},
    {65, 0xFF452D},
    {130, 0xDF2D48},
    {195, 0xFF5267},
    {255, 0xDF0D11},
}};

inline constexpr StopDataArray<6> Aurora = {{
    {0, 0x01052D},
    {64, 0x00C817},
    {128, 0x00FF00},
    {170, 0x00F32D},
    {200, 0x008707},
    {255, 0x01052D},
}};

inline constexpr StopDataArray<8> Atlantica = {{
    {0, 0x001C70},
    {1, 0x463220},
    {96, 0xFF80C6},
    {0, 0xF32D96},
    {12, 0x5F5200},
    {5, 0x34C819},
    {190, 0x5F1305},
    {255, 0x28AA50},
}};

inline constexpr StopDataArray<10> C92 = {{
    {0, 0x067E02},
    {45, 0x067E02},
    {45, 0x041E72},
    {90, 0x041E72},
    {90, 0xFF0500},
    {135, 0xFF0500},
    {135, 0xC43902},
    {180, 0xC43902},
    {180, 0x895502},
    {255, 0x895502},
}};

inline constexpr StopDataArray<8> C9New = {{
    {0, 0xFF0500},
    {60, 0xFF0500},
    {60, 0xC43902},
    {61, 0x78C439},
    {2, 0x78067E},
    {2, 0x7EB406},
    {126, 0x02B404},
    {30, 0x72BFFF},
}};

inline constexpr StopDataArray<18> Temperature = {{
    {0, 0x011B69},
    {14, 0x01287F},
    {28, 0x0146A8},
    {42, 0x015CC5},
    {56, 0x0177DD},
    {70, 0x038297},
    {84, 0x179C95},
    {99, 0x43B670},
    {113, 0x79C934},
    {127, 0x8ECB0B},
    {141, 0xE0DF01},
    {155, 0xFCBB02},
    {170, 0xF79301},
    {184, 0xED5701},
    {198, 0xE52B01},
    {226, 0xAB0202},
    {240, 0x500303},
    {255, 0x500303},
}};

inline constexpr StopDataArray<5> Aurora2 = {{
    {0, 0x11B10D},
    {64, 0x79F205},
    {128, 0x19AD79},
    {192, 0xFA4D7F},
    {255, 0xAB65DD},
}};

inline constexpr StopDataArray<3> RetroClown = {{
    {0, 0xE36503},
    {117, 0xC21213},
    {255, 0x5C08C0},
}};

inline constexpr StopDataArray<5> Candy = {{
    {0, 0xE5E301},
    {15, 0xE36503},
    {142, 0x280150},
    {198, 0x11014F},
    {255, 0x00002D},
}};

inline constexpr StopDataArray<2> ToxyReaf = {{
    {0, 0x01DD35},
    {255, 0x4903B2},
}};

inline constexpr StopDataArray<4> FairyReaf = {{
    {0, 0xB80180},
    {160, 0x01C1B6},
    {219, 0x99E3BE},
    {255, 0xFFFFFF},
}};

inline constexpr StopDataArray<9> SemiBlue = {{
    {0, 0x000000},
    {12, 0x010103},
    {53, 0x080116},
    {80, 0x040659},
    {119, 0x0219D8},
    {145, 0x070A63},
    {186, 0x0F021F},
    {233, 0x020105},
    {255, 0x000000},
}};

inline constexpr StopDataArray<8> PinkCandy = {{
    {0, 0xFFFFFF},
    {45, 0x070CFF},
    {112, 0xE3017F},
    {112, 0xE3017F},
    {140, 0xFFFFFF},
    {155, 0xE3017F},
    {196, 0x2D0163},
    {255, 0xFFFFFF},
}};

inline constexpr StopDataArray<4> RedReaf = {{
    {0, 0x030D2B},
    {104, 0x4E8DF0},
    {188, 0xFF0000},
    {255, 0x1C0101},
}};

inline constexpr StopDataArray<7> AquaFlash = {{
    {0, 0x000000},
    {66, 0x39E3E9},
    {96, 0xFFFF08},
    {124, 0xFFFFFF},
    {153, 0xFFFF08},
    {188, 0x39E3E9},
    {255, 0x000000},
}};

inline constexpr StopDataArray<7> YelbluHot = {{
    {0, 0x040209},
    {58, 0x10002F},
    {122, 0x180010},
    {158, 0x900901},
    {183, 0xB32D01},
    {219, 0xDC7202},
    {255, 0xEAED01},
}};

inline constexpr StopDataArray<6> LiteLight = {{
    {0, 0x000000},
    {9, 0x010101},
    {40, 0x050506},
    {66, 0x050506},
    {101, 0x0A010C},
    {255, 0x000000},
}};

inline constexpr StopDataArray<5> RedFlash = {{
    {0, 0x000000},
    {99, 0xE30101},
    {130, 0xF9C75F},
    {155, 0xE30101},
    {255, 0x000000},
}};

inline constexpr StopDataArray<8> BlinkRed = {{
    {0, 0x010101},
    {43, 0x04010B},
    {76, 0x0A0103},
    {109, 0xA1041D},
    {127, 0xFF567B},
    {165, 0x7D10A0},
    {204, 0x230DDF},
    {255, 0x120212},
}};

inline constexpr StopDataArray<7> RedShift = {{
    {0, 0x1F011B},
    {45, 0x220110},
    {99, 0x890509},
    {132, 0xD5800A},
    {175, 0xC71601},
    {201, 0xC70906},
    {255, 0x010001},
}};

inline constexpr StopDataArray<11> RedTide = {{
    {0, 0xF70500},
    {28, 0xFF4301},
    {43, 0xEA580B},
    {58, 0xEAB033},
    {84, 0xE51C01},
    {114, 0x710C01},
    {140, 0xFFE12C},
    {168, 0x710C01},
    {196, 0xF4D158},
    {216, 0xFF1C01},
    {255, 0x350101},
}};

inline constexpr StopDataArray<10> Candy2 = {{
    {0, 0x272122},
    {25, 0x04060F},
    {48, 0x311D16},
    {73, 0xE0AD01},
    {89, 0xB12305},
    {130, 0x04060F},
    {163, 0xFF7206},
    {186, 0xE0AD01},
    {211, 0x272122},
    {255, 0x010101},
}};

inline constexpr StopDataArray<3> AudioResponsiveRatio = {{
    {0, 0xFF0000},
    {125, 0x00FF00},
    {255, 0x0000FF},
}};

inline constexpr StopDataArray<3> AudioResponsiveHue = {{
    {0, 0xFF0000},
    {125, 0x00FF00},
    {255, 0x0000FF},
}};

inline constexpr StopDataArray<3> AudioResponsiveRamp = {{
    {0, 0xFF0000},
    {125, 0x00FF00},
    {255, 0x0000FF},
}};

template <typename TColor, const auto& TData> struct PaletteFactory
{
    inline static constexpr auto stops = []()
    {
        std::array<colors::palettes::PaletteStop<TColor>, TData.size()> result{};
        for (size_t index = 0; index < TData.size(); ++index)
        {
            result[index] = colors::palettes::PaletteStop<TColor>::fromRgb8(TData[index].first, TData[index].second);
        }
        return result;
    }();

    static colors::palettes::StaticPalette<TColor> create() { return colors::palettes::StaticPalette<TColor>(stops); }
};
} // namespace default_palette_data

template <typename TColor = colors::DefaultColorType> inline span<const NamedPalette<TColor>> StaticPalettes()
{
    static constexpr std::array<NamedPalette<TColor>, 68> palettes = {
        NamedPalette<TColor>{"Party",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Party>::create},
        NamedPalette<TColor>{"Cloud",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Cloud>::create},
        NamedPalette<TColor>{"Lava", &default_palette_data::PaletteFactory<TColor, default_palette_data::Lava>::create},
        NamedPalette<TColor>{"Ocean",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Ocean>::create},
        NamedPalette<TColor>{"Forest",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Forest>::create},
        NamedPalette<TColor>{"Rainbow",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Rainbow>::create},
        NamedPalette<TColor>{"Rainbow Bands",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::RainbowBands>::create},
        NamedPalette<TColor>{"Sunset",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Sunset>::create},
        NamedPalette<TColor>{"Rivendell",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Rivendell>::create},
        NamedPalette<TColor>{"Breeze",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Breeze>::create},
        NamedPalette<TColor>{"Red & Blue",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::RedBlue>::create},
        NamedPalette<TColor>{"Yellowout",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Yellowout>::create},
        NamedPalette<TColor>{"Analogous",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Analogous>::create},
        NamedPalette<TColor>{"Splash",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Splash>::create},
        NamedPalette<TColor>{"Pastel",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Pastel>::create},
        NamedPalette<TColor>{"Sunset 2",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Sunset2>::create},
        NamedPalette<TColor>{"Beach",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Beach>::create},
        NamedPalette<TColor>{"Vintage",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Vintage>::create},
        NamedPalette<TColor>{"Departure",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Departure>::create},
        NamedPalette<TColor>{"Landscape",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Landscape>::create},
        NamedPalette<TColor>{"Beech",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Beech>::create},
        NamedPalette<TColor>{"Sherbet",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Sherbet>::create},
        NamedPalette<TColor>{"Hult", &default_palette_data::PaletteFactory<TColor, default_palette_data::Hult>::create},
        NamedPalette<TColor>{"Hult 64",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Hult64>::create},
        NamedPalette<TColor>{"Drywet",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Drywet>::create},
        NamedPalette<TColor>{"Jul", &default_palette_data::PaletteFactory<TColor, default_palette_data::Jul>::create},
        NamedPalette<TColor>{"Grintage",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Grintage>::create},
        NamedPalette<TColor>{"Rewhi",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Rewhi>::create},
        NamedPalette<TColor>{"Tertiary",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Tertiary>::create},
        NamedPalette<TColor>{"Fire", &default_palette_data::PaletteFactory<TColor, default_palette_data::Fire>::create},
        NamedPalette<TColor>{"Icefire",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Icefire>::create},
        NamedPalette<TColor>{"Cyane",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Cyane>::create},
        NamedPalette<TColor>{"Light Pink",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::LightPink>::create},
        NamedPalette<TColor>{"Autumn",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Autumn>::create},
        NamedPalette<TColor>{"Magenta",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Magenta>::create},
        NamedPalette<TColor>{"Magred",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Magred>::create},
        NamedPalette<TColor>{"Yelmag",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Yelmag>::create},
        NamedPalette<TColor>{"Yelblu",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Yelblu>::create},
        NamedPalette<TColor>{"Orange & Teal",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::OrangeTeal>::create},
        NamedPalette<TColor>{"Tiamat",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Tiamat>::create},
        NamedPalette<TColor>{"April Night",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::AprilNight>::create},
        NamedPalette<TColor>{"Orangery",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Orangery>::create},
        NamedPalette<TColor>{"C9", &default_palette_data::PaletteFactory<TColor, default_palette_data::C9>::create},
        NamedPalette<TColor>{"Sakura",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Sakura>::create},
        NamedPalette<TColor>{"Aurora",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Aurora>::create},
        NamedPalette<TColor>{"Atlantica",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Atlantica>::create},
        NamedPalette<TColor>{"C9 2", &default_palette_data::PaletteFactory<TColor, default_palette_data::C92>::create},
        NamedPalette<TColor>{"C9 New",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::C9New>::create},
        NamedPalette<TColor>{"Temperature",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Temperature>::create},
        NamedPalette<TColor>{"Aurora 2",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Aurora2>::create},
        NamedPalette<TColor>{"Retro Clown",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::RetroClown>::create},
        NamedPalette<TColor>{"Candy",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Candy>::create},
        NamedPalette<TColor>{"Toxy Reaf",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::ToxyReaf>::create},
        NamedPalette<TColor>{"Fairy Reaf",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::FairyReaf>::create},
        NamedPalette<TColor>{"Semi Blue",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::SemiBlue>::create},
        NamedPalette<TColor>{"Pink Candy",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::PinkCandy>::create},
        NamedPalette<TColor>{"Red Reaf",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::RedReaf>::create},
        NamedPalette<TColor>{"Aqua Flash",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::AquaFlash>::create},
        NamedPalette<TColor>{"Yelblu Hot",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::YelbluHot>::create},
        NamedPalette<TColor>{"Lite Light",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::LiteLight>::create},
        NamedPalette<TColor>{"Red Flash",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::RedFlash>::create},
        NamedPalette<TColor>{"Blink Red",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::BlinkRed>::create},
        NamedPalette<TColor>{"Red Shift",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::RedShift>::create},
        NamedPalette<TColor>{"Red Tide",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::RedTide>::create},
        NamedPalette<TColor>{"Candy2",
                             &default_palette_data::PaletteFactory<TColor, default_palette_data::Candy2>::create},
        NamedPalette<TColor>{
            "Audio Responsive Ratio",
            &default_palette_data::PaletteFactory<TColor, default_palette_data::AudioResponsiveRatio>::create},
        NamedPalette<TColor>{
            "Audio Responsive Hue",
            &default_palette_data::PaletteFactory<TColor, default_palette_data::AudioResponsiveHue>::create},
        NamedPalette<TColor>{
            "Audio Responsive Ramp",
            &default_palette_data::PaletteFactory<TColor, default_palette_data::AudioResponsiveRamp>::create},
    };
    return span<const NamedPalette<TColor>>(palettes.data(), palettes.size());
}

} // namespace lw::palettes
