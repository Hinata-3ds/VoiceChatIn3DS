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
#define SERVER_IP "192.168.1.100" // サーバーのIPアドレスに置き換える
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

        // サーバーに接続
        if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
            OSD::Notify("Connection failed");
            close(sockfd);
            return;
        }

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
                ssize_t sentBytes = send(sockfd, micBuffer, sampleDataSize, 0);

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
    }

    void VoiceChatServer(MenuEntry *entry) {
        u32 *socBuffer;
        u32 socBufferSize = 0x100000;
        Result ret = 0;
        ret = svcControlMemoryUnsafe((u32 *)(&socBuffer), processMemoryAddr, socBufferSize, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(ret)) {
            MessageBox(Utils::Format("Error allocating memory: %08X\n", ret))();
            return;
        }

        ret = socInit(socBuffer, socBufferSize);
        if (R_FAILED(ret)) {
            socExit();
            MessageBox(Utils::Format("Error initializing SOC service: %08X\n", ret))();
            svcControlMemoryUnsafe((u32 *)(&socBuffer), processMemoryAddr, socBufferSize, MEMOP_FREE, static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
            return;
        }
        else
            MessageBox("socInit success")();

        // ソケットの作成
        int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == -1) {
            MessageBox("Error creating socket")();
            svcControlMemoryUnsafe(socBuffer, processMemoryAddr, socBufferSize, static_cast<MemOp>(MEMOP_FREE | MEMOP_REGION_SYSTEM), static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
            return;
        }
        else
            MessageBox("Socket creation successful!")();

        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
            MessageBox("Error binding socket")();
            close(sockfd);
            return;
        }
        else
            MessageBox("Binding successful!")();

        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int new_sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);
        if (new_sockfd == -1) {
            MessageBox("Error accepting connection")();
            close(sockfd);
            return;
        }

        MessageBox("Connection established")();

        close(new_sockfd);
        close(sockfd);
    }
}
