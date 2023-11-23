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
#define PORT 5000
#define SERVER_IP "127.0.0.1" // サーバーのIPアドレスに置き換える
u32 processMemoryAddr = 0x6500000;

namespace CTRPluginFramework
{

void VoiceChatServerLoop(void *arg) {
    int sockfd = *reinterpret_cast<int*>(arg);

    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int new_sockfd;

    // 接続が来るまで待機する
    while ((new_sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen)) == -1) {
        if (errno != EINTR) {
            MessageBox("Error accepting connection")();
            close(sockfd);
            return;
        }
    }

    MessageBox("Connection established")();

    close(new_sockfd);
}

void VoiceChatClientLoop(void *arg) {
    int sockfd = *(static_cast<int *>(arg));

    u8 micBuffer[BUFFER_SIZE] __attribute__((aligned(0x1000)));

    // 音声の取得
    Result micResult = micInit(micBuffer, BUFFER_SIZE);
    if (R_FAILED(micResult)) {
        MessageBox(Utils::Format("Error initializing microphone: %08X\n", micResult))();
        close(sockfd);
        return;
    }

    // マイクのサンプリング開始
    MICU_StartSampling(MICU_ENCODING_PCM16, MICU_SAMPLE_RATE_32730, 0, BUFFER_SIZE, false);

    bool isRunning = true;
    while (isRunning) {
        Controller::Update();

        // Bボタンが押されているかどうかを確認
        if (Controller::IsKeyDown(Key::B)) {
            // マイクから音声データを読み取り
            u32 sampleDataSize = micGetSampleDataSize();
            u32 lastSampleOffset = micGetLastSampleOffset();
            memcpy(micBuffer, micBuffer + lastSampleOffset, sampleDataSize);

            // サーバーに音声データを送信
            ssize_t sentBytes = send(sockfd, micBuffer, sampleDataSize, 0);

            if (sentBytes == -1) {
                MessageBox("Error sending data")();
            }
        } else {
            // Bボタンが離されたらマイクのサンプリング停止
            MICU_StopSampling();
            isRunning = false; // ボタンが離されたらループを終了
        }
    }

    MICU_StopSampling();
    micExit();
    close(sockfd);
}

void VoiceChatClient(MenuEntry *entry) {
    Keyboard kb("Input Server IP Address");

    // キーボードを開き、ユーザーにサーバーのIPアドレスを入力させる
    std::string serverIP;
    int result = kb.Open(serverIP);
    if (result == -1 || result == -2) {
        // キャンセルまたはエラーが発生した場合の処理
        MessageBox("Canceled or Error occurred")();
        return;
    }

    u32 *socBuffer;
    u32 socBufferSize = 0x100000;
    Result ret = 0;
    ret = svcControlMemoryUnsafe((u32 *)(&socBuffer), processMemoryAddr, socBufferSize, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
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
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        MessageBox("Error creating socket")();
        return;
    }
    else
        MessageBox("Socket creation successful!");

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str()); // 入力されたIPアドレスを設定

    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        MessageBox("Error connecting to server")();
        close(sockfd);
        return;
    }
    else
        MessageBox("Connected to server!");

    RGBLedPattern blueLED; // 青色の LED パターンを作成

    // 青色の LED パターンの設定
    LED_Color blueColor(0, 0, 255); // 青色の RGB 値 (R:0, G:0, B:255)
    float delay_time = 0.2f; // パターンの変化の遅延時間
    float loop_delay = 0.5f; // パターンのループ遅延時間
    blueLED = LED::GeneratePattern(blueColor, LED_PatType::CONSTANT, delay_time, loop_delay);

    // Bボタンが押されているかどうかを確認
    ThreadEx clientThread(VoiceChatClientLoop, 4096, 0x30, -1);
    clientThread.Start(&sockfd);

    // ソケットのクローズ
    MICU_StopSampling();
    micExit();
    close(sockfd);
}       

void VoiceChatServer(MenuEntry *entry) {
    Keyboard kb("Input IP Address");

    // キーボードを開き、ユーザーにIPアドレスを入力させる
    std::string ipAddress;
    int result = kb.Open(ipAddress);
    if (result == -1 || result == -2) {
        // キャンセルまたはエラーが発生した場合の処理
        MessageBox("Canceled or Error occurred")();
        return;
    }
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
      // ソケットの作成
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        MessageBox("Error creating socket")();
        svcControlMemoryUnsafe(socBuffer, processMemoryAddr, socBufferSize, static_cast<MemOp>(MEMOP_FREE | MEMOP_REGION_SYSTEM), static_cast<MemPerm>(MEMPERM_READ | MEMPERM_WRITE));
        return;
    }
    else
        MessageBox("Socket creation successful!");

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str()); // 

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        MessageBox("Error binding socket")();
        close(sockfd);
        return;
    }
    else
        MessageBox("Binding successful!");

    if (listen(sockfd, 1) == -1) {
        MessageBox("Error listening for connections")();
        close(sockfd);
        return;
    }
    else
        MessageBox("Listening now");

    // スレッドを作成し、ループ関数を実行
    ThreadEx serverThread(VoiceChatServerLoop, 4096, 0x30, -1);
    serverThread.Start(&sockfd);

    // 何か追加の処理があればここに記述

    // ソケットをクローズしないようにするか、スレッドが終了するまで待機する必要があります
    // close(sockfd);
    //serverThread.Join(true);
}
}
