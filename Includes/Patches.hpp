#pragma once

#include <CTRPluginFramework.hpp>

namespace CTRPluginFramework
{
    static constexpr auto SHAREDMEM_ADDR = 0x7500000;
    static constexpr auto SERVICE_SHAREDMEM_SIZE = 0x20000;
    static constexpr auto SERVICE_COUNT = 1;
    static constexpr auto SOCKET_SHAREDMEM_ADDR = SHAREDMEM_ADDR;

    bool InstallSocketHooks(void);
}