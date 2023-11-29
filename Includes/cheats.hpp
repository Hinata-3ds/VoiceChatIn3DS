#ifndef CHEATS_H
#define CHEATS_H

#include <CTRPluginFramework.hpp>
#include "Helpers.hpp"
#include "Unicode.h"

extern u8 *soundBuffer;
constexpr u32 SOUND_BUFFER_ADDR = 0x6500000;
constexpr u32 SOUND_BUFFER_SIZE = 0x100000;

extern u8 *micBuffer;
constexpr u32 MIC_BUFFER_ADDR = 0x7700000;
constexpr u32 MIC_BUFFER_SIZE = 0x30000;
namespace CTRPluginFramework
{
    using StringVector = std::vector<std::string>;
    void InputIPAddressAndPort( MenuEntry *entry );
    void VoiceChatServer( MenuEntry *entry );
    void VoiceChatClient( MenuEntry *entry );
    
}
#endif
