#include <3ds.h>
#include "csvc.h"
#include <CTRPluginFramework.hpp>
#include "cheats.hpp"
#include <vector>
#include "ctr-led-brary.hpp"
#include "Patches.hpp"
#include "Debug.hpp"

u8 *soundBuffer = nullptr;
u8 *micBuffer = nullptr;

namespace CTRPluginFramework
{
    static void    ToggleTouchscreenForceOn(void) {
        static u32 original = 0;
        static u32 *patchAddress = nullptr;

        if (patchAddress && original) {
            *patchAddress = original;
            return;
        }

        static const std::vector<u32> pattern = {
            0xE59F10C0, 0xE5840004, 0xE5841000, 0xE5DD0000,
            0xE5C40008, 0xE28DD03C, 0xE8BD80F0, 0xE5D51001,
            0xE1D400D4, 0xE3510003, 0x159F0034, 0x1A000003
        };

        Result  res;
        Handle  processHandle;
        s64     textTotalSize = 0;
        s64     startAddress = 0;
        u32 *   found;

        if (R_FAILED(svcOpenProcess(&processHandle, 16)))
            return;

        svcGetProcessInfo(&textTotalSize, processHandle, 0x10002);
        svcGetProcessInfo(&startAddress, processHandle, 0x10005);
        if(R_FAILED(svcMapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x14000000, processHandle, (u32)startAddress, textTotalSize)))
            goto exit;

        found = (u32 *)Utils::Search<u32>(0x14000000, (u32)textTotalSize, pattern);

        if (found != nullptr) {
            original = found[13];
            patchAddress = (u32 *)PA_FROM_VA((found + 13));
            found[13] = 0xE1A00000;
        }

        svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x14000000, textTotalSize);
exit:
        svcCloseHandle(processHandle);
    }

    void    ProcessEventCallback(Process::Event event) {

        if (event == Process::Event::EXIT) {
            socExit();

            u32 temp;
            svcControlMemoryUnsafe((u32 *)&temp, SHAREDMEM_ADDR, SERVICE_SHAREDMEM_SIZE * SERVICE_COUNT, (MemOp)MEMOP_FREE, (MemPerm)0);
        }
    }
   

    static    u32 *socBuffer = nullptr;
    constexpr u32 SOC_BUFFER_ADDR = 0x7500000;
    constexpr u32 SOC_BUFFER_SIZE = 0x100000;

    void    PatchProcess(FwkSettings &settings) {

        Process::SetProcessEventCallback(ProcessEventCallback);

        if(System::IsCitra()) {
            Result res;

            res = socInit((u32 *)0, SERVICE_SHAREDMEM_SIZE);
            DEBUG_NOTIFY(Utils::Format("Soc Init: %08lX", res));
        } else {
            u32 temp;
            Result res;

            if(R_FAILED(res = svcControlMemoryUnsafe((u32 *)&temp, SHAREDMEM_ADDR, SERVICE_SHAREDMEM_SIZE * SERVICE_COUNT, MemOp(MEMOP_REGION_SYSTEM | MEMOP_ALLOC), MemPerm(MEMPERM_READ | MEMPERM_WRITE))))
            {
                MessageBox(Utils::Format("Failed to allocate memory. (Error code: %08lX)\nWe are going to abort.", res));
                abort();
            }

            res = socInit((u32 *)SHAREDMEM_ADDR, SERVICE_SHAREDMEM_SIZE);
            DEBUG_NOTIFY(Utils::Format("socInit: %08lX", res));
        }

        if (!InstallSocketHooks()) {
            OSD::Notify("Failed to install socket hooks", Color::Red);
        }
    }

    
    void SetCustomTheme() {
        FwkSettings& settings = FwkSettings::Get();

        settings.WindowTitleColor = Color(32, 229, 156);
        settings.MenuSelectedItemColor = Color(224, 61, 52);
        settings.BackgroundBorderColor = Color(32, 229, 156);
        settings.MenuUnselectedItemColor = Color(32, 229, 156);
    }


    void InitializeSockets() {
        Result ret = RL_SUCCESS;

        ret = svcControlMemoryUnsafe((u32 *)&soundBuffer, SOUND_BUFFER_ADDR, SOUND_BUFFER_SIZE, MemOp(MEMOP_REGION_SYSTEM | MEMOP_ALLOC), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(ret) || !soundBuffer) 
            OSD::Notify("alloc soundBuffer failed");
        else
            OSD::Notify("alloc soundBuffer success");

        ret = svcControlMemoryUnsafe((u32 *)&micBuffer, MIC_BUFFER_ADDR, MIC_BUFFER_SIZE, MemOp(MEMOP_REGION_SYSTEM | MEMOP_ALLOC), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(ret) || !micBuffer)
            OSD::Notify("alloc micBuffer failed");
        else
            OSD::Notify("alloc micBuffer success");
    }

    void InitMenu(PluginMenu &menu) {
        MenuFolder* folder = new MenuFolder("システム");
        *folder += new MenuEntry("[IP:Port]", nullptr, InputIPAddressAndPort);
        *folder += new MenuEntry("[Server]", nullptr, voiceChatServer);
        *folder += new MenuEntry("[Client]", nullptr, voiceChatClient);
        menu += folder;
    }

    void EventCallback(Process::Event event) {
        if (event == Process::Event::EXIT) {
            if (System::IsCitra()) {
                free(socBuffer);
            } else {
                MemInfo info;
                PageInfo out;
                svcQueryMemory(&info, &out, SOUND_BUFFER_ADDR);
                if (info.state != MemState::MEMSTATE_FREE)
                svcControlMemoryUnsafe(nullptr, SOUND_BUFFER_ADDR, SOUND_BUFFER_SIZE, MEMOP_FREE, MemPerm(0));
                svcQueryMemory(&info, &out, MIC_BUFFER_ADDR);
                if (info.state != MemState::MEMSTATE_FREE)
                svcControlMemoryUnsafe(nullptr, MIC_BUFFER_ADDR, MIC_BUFFER_SIZE, MEMOP_FREE, MemPerm(0));
                micExit();
            }
        }
    }

    int main(void) {
        PluginMenu *menu = new PluginMenu("Echo3DS", 0, 0, 0,
                                            "");
        FwkSettings::SetThemeDefault();

        SetCustomTheme();
        
        menu->SynchronizeWithFrame(true);

        InitMenu(*menu);
        InitializeSockets();
        
        Process::SetProcessEventCallback(EventCallback);
        menu->Run();

        delete menu;
        return 0;
    }
}
