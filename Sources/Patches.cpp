#include "Patches.hpp"
#include "Debug.hpp"

namespace CTRPluginFramework
{
    extern "C" Handle SOCU_handle;

    static Result SOCU_Shutdown(void)
    {
        Result ret = 0;
        u32 *cmdbuf = getThreadCommandBuffer();

        cmdbuf[0] = IPC_MakeHeader(0x19,0,0); // 0x190000

        ret = svcSendSyncRequest(SOCU_handle);
        if(ret != 0) {
            return ret;
        }

        return cmdbuf[1];
    }

    static u32 SocketInitHook(u32 mem, u32 size)
    {
        // TODO: different implementation is needed
        //       because SOCU_Shutdown closes all sockets.
        Result res = SOCU_Shutdown();

        DEBUG_NOTIFY(Utils::Format("3GX SOCU_Shutdown: %08lX", res));
        DEBUG_NOTIFY("Game socInit has been called");
        DEBUG_NOTIFY(Utils::Format("Memory: %08lX, Size: %08lX", mem, size));

        return HookContext::GetCurrent().OriginalFunction<u32>(mem, size);
    }

    static u32 SocketExitHook(u32 returnVal)
    {
        Result res;

        // Close 3gx socket
        socExit();

        // Re-initialize 3gx socket
        res = socInit((u32 *)SOCKET_SHAREDMEM_ADDR, SERVICE_SHAREDMEM_SIZE);

        DEBUG_NOTIFY(Utils::Format("3GX SOCU_Initialize: %08lX", res));
        DEBUG_NOTIFY("Game socExit has been called");

        return HookContext::GetCurrent().OriginalFunction<u32>(returnVal);
    }

    bool InstallSocketHooks(void)
    {
        static std::vector<u32> socInitPattern = {
            0xE92D4070,
            0xE1A05000,
            0xE1A04001,
            0xE59F10A4,
            0xE59F00A4,
            0xE24DD008
        };

        static std::vector<u32> socExitPattern = {
            0xE59F003C,
            0xE92D4038,
            0xE3A05000,
            0xE5C05000,
            0xE5900004,
            0xE58D0000,
            0xE1A0000D
        };

        Hook initHook;
        Hook exitHook;

        u32 addr = Utils::Search(0x00100000, Process::GetTextSize(), socInitPattern);

        if (addr)
        {
            initHook.InitializeForMitm(addr, (u32)SocketInitHook);
            initHook.Enable();

            DEBUG_NOTIFYC("Game socInit hook has been successfully installed", Color::Lime);
        }
        else
        {
            DEBUG_NOTIFYC("Game socInit not found", Color::Red);
            return false;
        }

        addr = Utils::Search(0x00100000, Process::GetTextSize(), socExitPattern);

        if (addr)
        {
            exitHook.InitializeForMitm(addr + 0x40, (u32)SocketExitHook);
            exitHook.Enable();

            DEBUG_NOTIFYC("Game socExit hook has been successfully installed", Color::Lime);
        }
        else
        {
            DEBUG_NOTIFYC("Game socExit not found", Color::Red);
            return false;
        }

        return true;
    }
}