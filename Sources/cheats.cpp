#include "cheats.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "csvc.h"
#include "C:/devkitPro/libctru/include/3ds/services/mic.h"
#include "Led.hpp"
#define BUFFER_SIZE 4096
//#define SERVER_IP "127.0.0.1" 

namespace CTRPluginFramework
{


std::string g_serverIP; 
int g_port = 0;
// プロトタイプ宣言
void VoiceChatClientLoop(void *arg);
void VoiceChatServerLoop(void *arg);


void InputIPAddressAndPort(MenuEntry *entry) {
    Keyboard kbIP("Input Server IP Address");
    Keyboard kbPort("Input Port Number");

    std::string serverIP, portStr;
    int resultIP = kbIP.Open(serverIP);
    if (resultIP == -1 || resultIP == -2) {
        MessageBox("Canceled or Error occurred while entering IP address")();
        return;
    }

    int resultPort = kbPort.Open(portStr);
    if (resultPort == -1 || resultPort == -2) {
        MessageBox("Canceled or Error occurred while entering Port number")();
        return;
    }

    g_serverIP = serverIP; // 入力されたIPアドレスを保存
    g_port = std::stoi(portStr); // ポート番号を保存

    MessageBox("IP Address and Port Number saved")();
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
        OSD::Notify("Socket creation successful!",Color::LimeGreen);

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(g_port);
    serverAddr.sin_addr.s_addr = inet_addr(g_serverIP.c_str());

    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        MessageBox("Error connecting to server")();
        return;
    }
    else
        OSD::Notify("Connected to server!",Color::LimeGreen);

    ThreadEx clientThread(VoiceChatClientLoop, 4096, 0x30, -1);
    clientThread.Start(&sockfd);
}

void VoiceChatServer(MenuEntry *entry) {
    Keyboard kb("Input IP Address and Port");
    std::string ipAndPort;
    int result = kb.Open(ipAndPort);
    if (result == -1 || result == -2) {
        MessageBox("Canceled or Error occurred")();
        return;
    }

    // 文字列からIPアドレスとポート番号を分割する
    size_t pos = ipAndPort.find(':');
    if (pos == std::string::npos) {
        MessageBox("Invalid input format")();
        return;
    }

    std::string ipAddress = ipAndPort.substr(0, pos);
    std::string portStr = ipAndPort.substr(pos + 1);

    int port = std::stoi(portStr);
    if (port < 0 || port > 65535) {
        MessageBox("Invalid port number")();
        return;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        OSD::Notify("Error creating socket", Color::Red);
        return;
    } else {
        OSD::Notify("Socket creation successful!", Color::LimeGreen);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        OSD::Notify("Error binding socket", Color::Red);
        close(sockfd);
        return;
    } else {
        OSD::Notify("Binding successful!", Color::LimeGreen);

        if (listen(sockfd, 1) == -1) {
            OSD::Notify("Error listening for connections", Color::Red);
            close(sockfd);
            return;
        } else {
            OSD::Notify("Listen successful!", Color::LimeGreen);

            ThreadEx serverThread(VoiceChatServerLoop, 4096, 0x30, -1);
            serverThread.Start(&sockfd);
        }
    }
}

void VoiceChatClientLoop(void *arg) {
    int sockfd = *(static_cast<int *>(arg));
    static u8 *micbuf = nullptr;
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
}

void VoiceChatServerLoop(void *arg) {
    int sockfd = *reinterpret_cast<int*>(arg);

    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int new_sockfd;

    // 接続が来るまで待機する
    while ((new_sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen)) == -1) {
        if (errno != EINTR) {
            OSD::Notify("Error accepting connection", Color::Red);
            close(sockfd);
            return;
        }
    }
    OSD::Notify("Connection established", Color::LimeGreen);
    // クライアントとの通信が終わったらソケットをクローズ
    close(new_sockfd);
}
}
