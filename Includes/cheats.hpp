#ifndef CHEATS_H
#define CHEATS_H

#include <CTRPluginFramework.hpp>
#include "Helpers.hpp"
#include "Unicode.h"

extern const u32 MIC_BUFFER_ADDR;
extern const u32 MIC_BUFFER_SIZE;
extern u8 *micBuffer;
namespace CTRPluginFramework
{
    using StringVector = std::vector<std::string>;
    void InputIPAddressAndPort( MenuEntry *entry );
    void VoiceChatServer( MenuEntry *entry );
    void VoiceChatClient( MenuEntry *entry );
    void bootMicInit(MenuEntry *entry);
}
#endif
