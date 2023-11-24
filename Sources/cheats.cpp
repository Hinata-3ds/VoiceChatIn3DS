#include "cheats.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "ctr-led-brary.hpp"
#include "csvc.h"
#include "C:/devkitPro/libctru/include/3ds/services/mic.h"
#include "Led.hpp"

#define BUFFER_SIZE 4096
#define PORT 5000
//#define SERVER_IP "127.0.0.1" 
u32 processMemoryAddr = 0x6500000;
u32 mic_buffer_addr1 = 0x680A2F3;
u32 mic_buffer_size1 = 0x30000;

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





void VoiceChatClientLoop(void *arg) {
    int sockfd = *(static_cast<int *>(arg));
    bool initialized = true;

    static u8 *micbuf = nullptr;
    Result ret = RL_SUCCESS;

    if (micbuf == nullptr) {
        // マイクバッファを初期化
        ret = svcControlMemoryUnsafe(reinterpret_cast<u32*>(&micbuf), mic_buffer_addr1, mic_buffer_size1, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        if (R_FAILED(ret)) {
            MessageBox("Error allocating memory for mic buffer")();
            return;
        }
        else{
            MessageBox("micInit success")();
        }
    }

    // マイクの初期化
    
    if (R_FAILED(micInit(micbuf, mic_buffer_size1))) {
        MessageBox(Utils::Format("Error initializing microphone"))();
        svcControlMemoryUnsafe((u32*)(&micbuf), mic_buffer_addr1, mic_buffer_size1, MEMOP_FREE, MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        return;
    }
    else {
        MessageBox("microphone successful!")();
        MICU_StartSampling(MICU_ENCODING_PCM16, MICU_SAMPLE_RATE_32730, 0, BUFFER_SIZE, false);
    }

    bool isRunning = true;
    Led led(nullptr); // 新しいLEDクラスのインスタンス化

    

    while (isRunning) {
        Controller::Update();

        if (Controller::IsKeyDown(Key::B)) {
            u32 sampleDataSize = micGetSampleDataSize();
            u32 lastSampleOffset = micGetLastSampleOffset();
            
            // 新しいマイクバッファにデータをコピー
            memcpy(micbuf, micbuf + lastSampleOffset, sampleDataSize);

            // 新しいLEDの色を設定（仮の値）
            led.setColor(255, 0, 255);
            led.update();

            ssize_t sentBytes = send(sockfd, micbuf, sampleDataSize, 0);
            if (sentBytes == -1) {
                MessageBox("Error sending data")();
                isRunning = false;  // エラーが発生したらループを抜ける
            }
        } if (Controller::IsKeyReleased(Key::B)){
            // LEDの停止
            led.setColor(0, 0, 0);
            led.update();

            MICU_StopSampling();
            isRunning = false;
        }
    }

    // 通信が終わったらソケットをクローズ
    //close(sockfd);

    //MICU_StopSampling();
    //micExit();
    svcControlMemoryUnsafe(reinterpret_cast<u32*>(&micbuf), mic_buffer_addr1, mic_buffer_size1, MEMOP_FREE, MemPerm(MEMPERM_READ | MEMPERM_WRITE));
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
        return;
    }
    else
        MessageBox("Connected to server!")();
        ThreadEx clientThread(VoiceChatClientLoop, 4096, 0x30, -1);
        clientThread.Start(&sockfd);

    // 接続成功時の処理、あとで
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

    OSD::Notify("Connection established", Color::LimeGreen);

    // クライアントとの通信が終わったらソケットをクローズ
    close(new_sockfd);
}

void VoiceChatClient(MenuEntry *entry) {
    // IPアドレスが入力されていない場合は処理しない
    if (g_serverIP.empty()) {
        MessageBox("IP Address not provided")();
        return;
    }

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
    serverAddr.sin_addr.s_addr = inet_addr(g_serverIP.c_str());

    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        MessageBox("Error connecting to server")();
        return;
    }
    else
        MessageBox("Connected to server!")();

    ThreadEx clientThread(VoiceChatClientLoop, 4096, 0x30, -1);
    clientThread.Start(&sockfd);
}

void VoiceChatServer(MenuEntry *entry) {
    Keyboard kb("Input IP Address");
    std::string ipAddress;
    int result = kb.Open(ipAddress);
    if (result == -1 || result == -2) {
        MessageBox("Canceled or Error occurred")();
        return;
    }

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
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());

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

    ThreadEx serverThread(VoiceChatServerLoop, 4096, 0x30, -1);
    serverThread.Start(&sockfd);
}
}
