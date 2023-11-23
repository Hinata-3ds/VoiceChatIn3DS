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
//#define SERVER_IP "127.0.0.1" 
u32 processMemoryAddr = 0x6500000;

namespace CTRPluginFramework
{

std::string g_serverIP; 

void InputIPAddress(MenuEntry *entry) {
    Keyboard kb("Input Server IP Address");

    std::string serverIP;
    int result = kb.Open(serverIP);
    if (result == -1 || result == -2) {
        MessageBox("Canceled or Error occurred")();
        return;
    }

    g_serverIP = serverIP; // 入力されたIPアドレスを保存
    MessageBox("IP Address saved")();
}




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
            u32 sampleDataSize = micGetSampleDataSize();
            u32 lastSampleOffset = micGetLastSampleOffset();
            memcpy(micBuffer, micBuffer + lastSampleOffset, sampleDataSize);

            // LEDを虹色に設定するパターンを生成
            RGBLedPattern rainbowPattern = LED::GeneratePattern(LED_Color(255, 0, 0), LED_PatType::ASCENDENT, 0.1f, 0.0f, 1, 2, 2, 2);
            // LEDのパターンを再生
            LED::PlayLEDPattern(rainbowPattern);

            // マイクから音声データを読み取り、サーバーに送信するコード
            // ...

            ssize_t sentBytes = send(sockfd, micBuffer, sampleDataSize, 0);
            if (sentBytes == -1) {
                MessageBox("Error sending data")();
            }
        } else {
            // Bボタンが離されたらLEDのパターンを停止
            LED::StopLEDPattern();

            // Bボタンが離されたらマイクのサンプリング停止
            MICU_StopSampling();
            isRunning = false; // ボタンが離されたらループを終了
        }
    }

    MICU_StopSampling();
    micExit();
    close(sockfd);
}

void ConnectToServer() {
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
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        MessageBox("Error creating socket")();
        return;
    }
    else
        MessageBox("Socket creation successful!")();

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(g_serverIP.c_str()); // 保存されたIPアドレスを設定

    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        MessageBox("Error connecting to server")();
        close(sockfd);
        return;
    }
    else
        MessageBox("Connected to server!")();
        ThreadEx clientThread(VoiceChatClientLoop, 4096, 0x30, -1);
        clientThread.Start(&sockfd);

    // 接続成功時の処理など
}

void VoiceChatClient(MenuEntry *entry) {

    // IPアドレスが入力されていない場合は処理しない
    if (g_serverIP.empty()) {
        MessageBox("IP Address not provided")();
        return;
    }

    ConnectToServer(); // 保存されたIPアドレスでサーバーに接続
    
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
        MessageBox("Socket creation successful!")();

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
        MessageBox("Binding successful!")();

    if (listen(sockfd, 1) == -1) {
        MessageBox("Error listening for connections")();
        close(sockfd);
        return;
    }
    else
        MessageBox("Listening now")();

    // スレッドを作成し、ループ関数を実行
    ThreadEx serverThread(VoiceChatServerLoop, 4096, 0x30, -1);
    serverThread.Start(&sockfd);

    // close(sockfd);
    //serverThread.Join(true);
}
}
