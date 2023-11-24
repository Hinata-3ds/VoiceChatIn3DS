#include <3ds.h>
#include "csvc.h"
#include <CTRPluginFramework.hpp>
#include "CTRPluginFramework/System/FwkSettings.hpp"
#include "CTRPluginFramework/Graphics/Color.hpp"
#include "cheats.hpp"

#include <vector>

#define BUFFER_SIZE 4096
#define PORT 5000
static u8 *micbuf = nullptr;
u32 mic_buffer_addr = 0x680A2F3;
u32 mic_buffer_size = 0x30000;
namespace CTRPluginFramework
{
    
    // This patch the NFC disabling the touchscreen when scanning an amiibo, which prevents ctrpf to be used
    static void    ToggleTouchscreenForceOn(void)
    {
        static u32 original = 0;
        static u32 *patchAddress = nullptr;

        if (patchAddress && original)
        {
            *patchAddress = original;
            return;
        }

        static const std::vector<u32> pattern =
        {
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

        if (found != nullptr)
        {
            original = found[13];
            patchAddress = (u32 *)PA_FROM_VA((found + 13));
            found[13] = 0xE1A00000;
        }

        svcUnmapProcessMemoryEx(CUR_PROCESS_HANDLE, 0x14000000, textTotalSize);
exit:
        svcCloseHandle(processHandle);
    }

    // This function is called before main and before the game starts
    // Useful to do code edits safely
    void    PatchProcess(FwkSettings &settings)
    {
        ToggleTouchscreenForceOn();
    }

    // This function is called when the process exits
    // Useful to save settings, undo patchs or clean up things
    void    OnProcessExit(void)
    {
        ToggleTouchscreenForceOn();
    }
    
    void SetCustomTheme() {
        FwkSettings& settings = FwkSettings::Get();

        // 各色を指定されたRGB値に変更する
        settings.WindowTitleColor = Color(32, 229, 156);
        settings.MenuSelectedItemColor = Color(224, 61, 52);
        settings.BackgroundBorderColor = Color(32, 229, 156);
        settings.MenuUnselectedItemColor = Color(32, 229, 156);
    }

    void InitializeSockets() {
        u32 processMemoryAddr = 0x6500000;
        u32 *socBuffer;
        u32 socBufferSize = 0x100000;
        Result ret = 0;
        static u8 *micbuf = nullptr;
        ret = svcControlMemoryUnsafe((u32 *)(&socBuffer), processMemoryAddr, socBufferSize, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(ret)) {
            socExit();
            MessageBox(Utils::Format("Error initializing SOC service: %08X\n", ret))();
            svcControlMemoryUnsafe((u32 *)(&socBuffer), processMemoryAddr, socBufferSize, MEMOP_FREE, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
            return;
        }
        else
            OSD::Notify("socInit success",Color::LimeGreen);

        Result micResult = RL_SUCCESS;
        

        if (micbuf == nullptr) {
              micResult = svcControlMemoryUnsafe((u32 *)(&micbuf), mic_buffer_addr, mic_buffer_size, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
            if (R_FAILED(micResult)) {
                MessageBox("Error allocating memory for mic buffer")();
                return;
            }
            else 
               OSD::Notify("alloc success!",Color::LimeGreen);
        }

        // マイクの初期化
        micResult = micInit(micbuf, mic_buffer_size);
        if (R_FAILED(micResult)) {
            MessageBox(Utils::Format("Error initializing microphone: %08X\n", micResult))();
           svcControlMemoryUnsafe((u32 *)(&micbuf), mic_buffer_addr, mic_buffer_size, MEMOP_FREE, MemPerm(MEMPERM_READ | MEMPERM_WRITE));
            return;
        }
        else {
            OSD::Notify("Microphone initialization successful!",Color::LimeGreen);
            MICU_StartSampling(MICU_ENCODING_PCM16, MICU_SAMPLE_RATE_32730, 0, BUFFER_SIZE, false);
        }
    }

    void    InitMenu(PluginMenu &menu)
    {
        MenuFolder* folder = new MenuFolder("システム");
            *folder += new MenuEntry("Input IP Address",nullptr,InputIPAddress),
            *folder += new MenuEntry("Server", nullptr, VoiceChatServer);
            *folder += new MenuEntry("connect", VoiceChatClient);
        menu += folder;
    }
    void EventCallback(Process::Event event){
        if (event == Process::Event::EXIT){
            micExit();
            svcControlMemoryUnsafe(reinterpret_cast<u32*>(&micbuf), mic_buffer_addr, mic_buffer_size, MEMOP_FREE, MemPerm(MEMPERM_READ | MEMPERM_WRITE));
            socExit();
        }
    }

    int     main(void)
    {
        PluginMenu *menu = new PluginMenu("Hinata", 0, 7, 4,
                                            "");
        FwkSettings::SetThemeDefault();

        // カスタムテーマを適用する
        SetCustomTheme();
        
        // Synnchronize the menu with frame event
        menu->SynchronizeWithFrame(true);

        // Init our menu entries & folders
        InitMenu(*menu);
        InitializeSockets();
        EventCallback(Process::Event::EXIT);
        Process::SetProcessEventCallback(EventCallback);
        // Launch menu and mainloop
        menu->Run();

        delete menu;
        //CleanupSockets();
        // Exit plugin
        return (0);
    }
}
