#include "cheats.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ctr-led-brary.hpp"
#include "csvc.h"
#include "C:/devkitPro/libctru/include/3ds/services/mic.h"

#define BUFFER_SIZE 4096
#define SOC_ALIGN  0x1000
#define PORT 59678
#define SERVER_IP "127.0.0.1" // サーバーのIPアドレスに置き換える
u32 processMemoryAddr = 0x6500000;

namespace CTRPluginFramework
{
    void VoiceChatClient(MenuEntry *entry) {
        // マイクの初期化
        u8 micBuffer[BUFFER_SIZE] __attribute__((aligned(0x1000)));
        Result micResult = micInit(micBuffer, BUFFER_SIZE);

        if (R_FAILED(micResult)) {
            OSD::Notify(Utils::Format("Error initializing microphone: %08X\n", micResult));
            return;
        }

        // ソケットの作成
        u32 *socBuffer;
        u32 socBufferSize = 0x1000; // 0x1000（ページサイズ）のバッファサイズを指定

        // SOCバッファの確保
        Result memResult = svcControlMemoryUnsafe(reinterpret_cast<u32 *>(&socBuffer), processMemoryAddr, socBufferSize, MEMOP_ALLOC, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(memResult)) {
            OSD::Notify(Utils::Format("Error allocating memory: %08X\n", memResult));
            return;
        }

        // SOCサービスの初期化
        Result socResult = socInit(socBuffer, socBufferSize);
        if (R_FAILED(socResult)) {
            OSD::Notify(Utils::Format("Error initializing SOC service: %08X\n", socResult));

            // SOCバッファの解放
            svcControlMemoryUnsafe(reinterpret_cast<u32 *>(socBuffer), processMemoryAddr, socBufferSize, MEMOP_FREE, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
            return;
        }
        
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            OSD::Notify("Error creating socket");
            return;
        }

        // サーバーのアドレス情報
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

        // マイクのサンプリング開始
        MICU_StartSampling(MICU_ENCODING_PCM16, MICU_SAMPLE_RATE_32730, 0, BUFFER_SIZE, false);
        
        RGBLedPattern blueLED; // 青色の LED パターンを作成

        // 青色の LED パターンの設定
        LED_Color blueColor(0, 0, 255); // 青色の RGB 値 (R:0, G:0, B:255)
        float delay_time = 0.2f; // パターンの変化の遅延時間
        float loop_delay = 0.5f; // パターンのループ遅延時間
        blueLED = LED::GeneratePattern(blueColor, LED_PatType::CONSTANT, delay_time, loop_delay);
        while (true) {
            // Bボタンが押されているかどうかを確認
            if (Controller::IsKeyDown(Key::B)) {
                // マイクから音声データを読み取り
                u32 sampleDataSize = micGetSampleDataSize();
                u32 lastSampleOffset = micGetLastSampleOffset();
                memcpy(micBuffer, micBuffer + lastSampleOffset, sampleDataSize);

                // サーバーにデータを送信
                ssize_t sentBytes = sendto(sockfd, micBuffer, sampleDataSize, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

                if (sentBytes == -1) {
                    OSD::Notify("Error sending data");
                    break;
                }
                LED::SwitchLEDPattern(blueLED);
            } else {
                // Bボタンが離されたらマイクのサンプリング停止
                MICU_StopSampling();
                LED::StopLEDPattern();
            }

            struct timespec sleepTime;
            sleepTime.tv_sec = 0;
            sleepTime.tv_nsec = 10000000;  // 10ミリ秒待機（nanosleepはナノ秒単位で待機）

            nanosleep(&sleepTime, NULL);
        }

        // ソケットのクローズ
        close(sockfd);

        // マイクのサンプリング停止
        MICU_StopSampling();
        Result socExitResult = socExit();
        if (R_FAILED(socExitResult)) {
            OSD::Notify(Utils::Format("Error closing SOC service: %08X\n", socExitResult));
            return;
        }

        // SOCバッファの解放
        Result memFreeResult = svcControlMemoryUnsafe(reinterpret_cast<u32 *>(socBuffer), processMemoryAddr, socBufferSize, MEMOP_FREE, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(memFreeResult)) {
            OSD::Notify(Utils::Format("Error freeing memory: %08X\n", memFreeResult));
            return;
        }
    }
    void VoiceChatServer(MenuEntry *entry) {
        u32 *socBuffer;
        u32 socBufferSize = 0x1000;
        Result memResult = svcControlMemoryUnsafe(socBuffer, processMemoryAddr, socBufferSize, MEMOP_ALLOC, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(memResult)) {
            OSD::Notify(Utils::Format("Error allocating memory: %08X\n", memResult));
            return;
        }
        Result socInitResult = socInit(reinterpret_cast<u32 *>(socBuffer), socBufferSize);
        if (R_FAILED(socInitResult)) {
            OSD::Notify(Utils::Format("Error initializing SOC service: %08X\n", socInitResult));
            svcControlMemoryUnsafe(reinterpret_cast<u32 *>(socBuffer), 0, socBufferSize, MEMOP_FREE, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
            return;
        }

        // ソケットの作成
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            OSD::Notify("Error creating socket");
            svcControlMemoryUnsafe(reinterpret_cast<u32 *>(socBuffer), processMemoryAddr, socBufferSize, MEMOP_FREE, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
            return;
        }

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
            OSD::Notify("Error binding socket");
            close(sockfd);
            return;
        }

        if (listen(sockfd, 5) == -1) {
            OSD::Notify("Error listening");
            close(sockfd);
            return;
        }

        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int new_sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);
        if (new_sockfd == -1) {
            OSD::Notify("Error accepting connection");
            close(sockfd);
            return;
        }

        OSD::Notify("Connection established");


        close(new_sockfd);
        close(sockfd);
    }
}