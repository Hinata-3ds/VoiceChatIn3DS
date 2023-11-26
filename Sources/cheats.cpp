
#include "cheats.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "csvc.h"
#include "C:/devkitPro/libctru/include/3ds/services/mic.h"
#include "Led.hpp"
#include "ncsnd.h"
#define BUFFER_SIZE 0x20000
//#define SERVER_IP "127.0.0.1" 

namespace CTRPluginFramework
{

std::string g_serverIP; 
int g_port = 0;
// プロトタイプ宣言
void SendVoiceData(void *arg);
void ProcessReceivedVoiceData(void *arg);

void InputIPAddressAndPort(MenuEntry *entry) {
    Keyboard kb("Input Server IP Address and Port");

    std::string ipAndPort;
    int result = kb.Open(ipAndPort);
    if(result == -1){
        MessageBox("Error occurred");
        return;
    }
    else if (result == -2) {
        MessageBox("Canceled")();
        return;
    }

    size_t pos = ipAndPort.find(':');
    if (pos == std::string::npos) {
        MessageBox("Invalid input format")();
        return;
    }

    std::string serverIP = ipAndPort.substr(0, pos);
    std::string portStr = ipAndPort.substr(pos + 1);

    // サーバーIPアドレスの保存
    g_serverIP = serverIP;

    // ポート番号の保存
    try {
        g_port = std::stoi(portStr);
    } catch (const std::exception &exp) {
        MessageBox("Invalid port number format")();
        return;
    }

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
    } else {
        OSD::Notify("Socket creation successful!", Color::LimeGreen);
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(g_port);
    serverAddr.sin_addr.s_addr = inet_addr(g_serverIP.c_str());

    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        MessageBox("Error connecting to server")();
        return;
    } else {
        // 接続が完了した際に接続した側のIPアドレスとポートを表示
        struct sockaddr_in addr;
        socklen_t addrLen = sizeof(addr);
        getsockname(sockfd, (struct sockaddr *)&addr, &addrLen);
        std::string connectedIP = inet_ntoa(addr.sin_addr);
        int connectedPort = ntohs(addr.sin_port);
        std::string message = "Connected to server! \nIP: " + connectedIP + " \nPort: " + std::to_string(connectedPort);
        MessageBox(message.c_str())();
    }

    ThreadEx clientSendThread(SendVoiceData, 4096, 0x30, -1);
    Result result = clientSendThread.Start(&sockfd);
    if (result != 0) {
        OSD::Notify("Error starting voice send thread",Color::Red);
        return;
    }
    else
        OSD::Notify("Thread success!",Color::LimeGreen);
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
        } 
        else 
            OSD::Notify("Listen successful!", Color::LimeGreen);
          
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int new_sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);
            if (new_sockfd == -1) {
                OSD::Notify("Error accepting connection", Color::Red);
                close(sockfd);
                return;
            } else {
                OSD::Notify("Connection established", Color::LimeGreen);
            }

            ThreadEx serverReceiveThread(ProcessReceivedVoiceData, 4096, 0x30, -1);
            Result result = serverReceiveThread.Start(&new_sockfd);
            if (result != 0) {
                MessageBox("Error starting voice receive thread")();
                return;
            }
            // クライアントとの通信が終わったらソケットをクローズ
            //close(new_sockfd);           
    }
}

void ProcessReceivedSound(u8* receivedSoundData, u32 receivedSoundSize) {
    // ncsndSound 構造体に受信した音声データをセットする
    ncsndSound receivedSound;
    ncsndInitializeSound(&receivedSound);
    

    receivedSound.isPhysAddr = true; // 物理アドレス設定
    receivedSound.sampleData = (u8*)svcConvertVAToPA((const void*)receivedSoundData, false); // 受信した音声データをセット
    receivedSound.totalSizeBytes = receivedSoundSize; // 受信した音声データのサイズをセット
    receivedSound.encoding = NCSND_ENCODING_PCM16; // 16ビットPCM
    receivedSound.sampleRate = 16360; // サンプリングレートを設定
    receivedSound.pan = 0.0;

    // サウンドを再生する
    if (R_FAILED(ncsndPlaySound(0x8, &receivedSound))) {
        OSD::Notify("Failed to play received sound", Color::Red);
    }
}

void SendVoiceData(void *arg) {
    int sockfd = *(static_cast<int *>(arg));
    u8 micBuffer[BUFFER_SIZE]; // マイクバッファ

    MICU_Encoding encoding = MICU_ENCODING_PCM16; // 16ビットPCM
    MICU_SampleRate sampleRate = MICU_SAMPLE_RATE_32730; // サンプリングレートを設定
    u32 micBuffer_datasize = micGetSampleDataSize();
    // マイクのサンプリングを開始
    Result result = MICU_StartSampling(encoding, sampleRate, 0, micBuffer_datasize, false);
    if (result != 0) {
        OSD::Notify("Failed to start sampling", Color::Red);
        micExit(); // マイクを終了する
        return;
    } else {
        OSD::Notify("Sampling success!", Color::LimeGreen);
    }

    Led led(nullptr); // LEDクラスのインスタンスを生成

    while (true) {
        // マイクから音声データを取得
        u32 sampleDataSize = micGetSampleDataSize();
        u32 lastSampleOffset = micGetLastSampleOffset();

        // 音声データをサーバーに送信
        ssize_t sentBytes = send(sockfd, micBuffer, sampleDataSize, 0);
        if (sentBytes == -1) {
            // データ送信に失敗した場合の処理
            OSD::Notify("Failed to send data", Color::Red);
            MICU_StopSampling(); // マイクのサンプリングを停止
            micExit(); // マイクを終了する
            break;
        } else {
            OSD::Notify("Send data success!");
        }

        // LEDを滑らかに点灯
        led.setSmoothing(0x0F); // スムージングを設定（滑らかさの度合い）
        led.setColor(255, 0, 255); // LEDの色を設定（赤色）
        led.update(); // LEDを更新

        // 任意の時間待機する(0.5)
        Sleep(Milliseconds(500));
        ThreadEx::Yield();
    }
}

void ProcessReceivedVoiceData(void *arg) {
    int sockfd = *(static_cast<int *>(arg));
    u8 receivedSoundBuffer[BUFFER_SIZE]; // 受信した音声データのバッファ7
    u32 sampleDataSize = micGetSampleDataSize();

    while (true) {
        // サーバーから音声データを受信
        ssize_t receivedBytes = recv(sockfd, receivedSoundBuffer, sampleDataSize, 0);
        if (receivedBytes == -1) {
            // データ受信に失敗した場合の処理
            OSD::Notify("Failed to receive data", Color::Red);
            close(sockfd); // ソケットを閉じる
            break;
        } else if (receivedBytes == 0) {
            // 接続が切断された場合の処理
            OSD::Notify("Connection closed", Color::Red);
            close(sockfd); // ソケットを閉じる
            break;
        }
        
        // 受信した音声データを処理・再生
        ProcessReceivedSound(receivedSoundBuffer, receivedBytes);
        ThreadEx::Yield();
    }
}
}
