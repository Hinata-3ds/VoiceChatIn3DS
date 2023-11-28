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
#include <vector>
#include <fcntl.h>
#define BUFFER_SIZE 0x20000
//#define SERVER_IP "127.0.0.1" 

int sockfd;

namespace CTRPluginFramework
{

std::string g_serverIP; 
int g_port = 0;
std::vector<std::string> console = {};

// プロトタイプ宣言
void ProcessReceivedVoiceData(void *arg);
bool CloseGameMicHandle(void);
void ProcessReceivedSound(u8* receivedSoundData, u32 receivedSoundSize);


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
    // entry->SetGameFunc(); 後で
}

void VoiceChatServer(MenuEntry *entry) {

    // Keyboard kb("Input IP Address and Port");
    // std::string ipAndPort;
    // int result = kb.Open(ipAndPort);
    // if (result == -1 || result == -2) {
    //     MessageBox("Canceled or Error occurred")();
    //     return;
    // }

    // // 文字列からIPアドレスとポート番号を分割する
    // size_t pos = ipAndPort.find(':');
    // if (pos == std::string::npos) {
    //     MessageBox("Invalid input format")();
    //     return;
    // }

    // std::string ipAddress = ipAndPort.substr(0, pos);
    // std::string portStr = ipAndPort.substr(pos + 1);

    // int port = std::stoi(portStr);
    // if (port < 0 || port > 65535) {
    //     MessageBox("Invalid port number")();
    //     return;
    // }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        OSD::Notify("Error creating socket\n");
        return;
    }

    if (g_serverIP.empty()) {
        MessageBox("IP Address not provided")();
        return;
    }

    OSD::Notify("Socket creation successful!\n");
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(g_port);
    serverAddr.sin_addr.s_addr = inet_addr(g_serverIP/*ipAddress*/.c_str());

    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        OSD::Notify("Error binding socket\n");
        close(sockfd);
        return;
    } else{
        OSD::Notify("Binding successful!\n");

        if (listen(sockfd, 1) < 0) {
            OSD::Notify("Error listening for connections\n");
            close(sockfd);
            return;
        } 
        else{
            OSD::Notify("Listen successful!\n");
          
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int new_sockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &clientLen);
            if (new_sockfd == -1) {
                OSD::Notify("Error accepting connection\n");
                close(sockfd);
                return;
            } else 
                OSD::Notify("Connection established\n");
            
            const Screen &screen = OSD::GetTopScreen();

            while (true) {
                // サイズの受信用バッファ
                u32     dataSize = 0;
                ssize_t receivedSize = recv(new_sockfd, &dataSize, sizeof(dataSize), 0);

                if (receivedSize <= 0) {
                    console.push_back("Failed to receive size");
                    close(sockfd);
                    return;
                } 
                //else 
                    //console.push_back(Utils::Format("Received size: %08lX", receivedSize));

                // 音声データを受信するバッファを動的に確保
                u8 *receivedSoundBuffer = new u8[receivedSize];

                // サーバーから音声データを受信
                ssize_t receivedBytes = recv(new_sockfd, receivedSoundBuffer, receivedSize, 0);
                if (receivedBytes <= 0) {
                    console.push_back("Failed to receive data.");
                    close(sockfd);
                    delete[] receivedSoundBuffer; // メモリ解放
                    return;
                }
                else{
                    ncsndSetVolume(0x8, 1, 0);
                    ncsndSetRate(0x8, 16360, 1);
                    ProcessReceivedSound(receivedSoundBuffer, receivedSize);
                }
                    while (11 < console.size())
                    console.erase(console.begin());

                    screen.DrawRect(30, 20, 340, 200, Color::Black);
                    screen.DrawRect(32, 22, 336, 196, Color::Magenta, false);
                    for (const auto &log : console)
                        screen.DrawSysfont(log, 35, 22 + (&log - &console[0]) * 18);

                    OSD::SwapBuffers();     
                }
            }
        }

     
}

void sendDataToserver(MenuEntry *entry) {
    if (g_serverIP.empty()) {
        MessageBox("IP Address not provided")();
        return;
    }

    u32 micBuffer_pos = 0;
    CloseGameMicHandle();
    Result ret = RL_SUCCESS;
    bool init = true;
    std::vector<std::string> consoleOfCTRPF = {};



    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        MessageBox("Error creating socket")();
        return;
    } 
    OSD::Notify("Socket creation successful!", Color::LimeGreen);

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(g_port);
    serverAddr.sin_addr.s_addr = inet_addr(g_serverIP.c_str());
    
    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        MessageBox("Error connecting to server")();
        return;
    } 
    // 接続が完了した際に接続した側のIPアドレスとポートを表示
    // struct sockaddr_in addr;
    // socklen_t addrLen = sizeof(addr);
    // getsockname(sockfd, (struct sockaddr *)&addr, &addrLen);
    // std::string connectedIP = inet_ntoa(addr.sin_addr);
    // int connectedPort = ntohs(addr.sin_port);
    // std::string message = "Connected to server! \nIP: " + connectedIP + " \nPort: " + std::to_string(connectedPort);
    // MessageBox(message.c_str())();

    if (R_FAILED(ncsndInit(false))) {
        

        consoleOfCTRPF.push_back("Failed to initialize sound.\n");
        init = false;
    }
    if (!micBuffer)
        ret = svcControlMemoryUnsafe((u32 *)&micBuffer, MIC_BUFFER_ADDR, MIC_BUFFER_SIZE, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
    
    if (R_FAILED(ret)) {
        consoleOfCTRPF.push_back("MIC buffer memory allocation failed.\n");
        init = false;
    }
    else if (R_FAILED(micInit(micBuffer, MIC_BUFFER_SIZE))) {
        consoleOfCTRPF.push_back("Failed to initialize MIC.\n");
        init = false;
    }
    u32 audioBuffer_pos = 0;
    if(!soundBuffer){
        ret = svcControlMemoryUnsafe((u32 *)&soundBuffer, SOUND_BUFFER_ADDR, SOUND_BUFFER_SIZE, MemOp(MEMOP_ALLOC | MEMREGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
    }
    else
        ret = RL_SUCCESS;
    if (R_FAILED(ret) || !soundBuffer) {

        consoleOfCTRPF.push_back("Failed to allocate memory for the sound buffer.\n");
        init = false;
    }

    Sleep(Milliseconds(500));

    if (init)
        consoleOfCTRPF.push_back("Speak while pressing A.\n");
    consoleOfCTRPF.push_back("Press B to exit.\n");

    const Screen &screen = OSD::GetTopScreen();

    while (true) {
        Controller::Update();
        if (init && Controller::IsKeyPressed(Key::A)) {
            micBuffer_pos = 0;
            audioBuffer_pos = 0;
            if (R_FAILED(MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_16360, 0, micGetSampleDataSize(), true)))
                consoleOfCTRPF.push_back("Sampling could not be initiated.\n");
            else
                consoleOfCTRPF.push_back("Sampling has begun.\n");
        }

        if (init && Controller::IsKeysDown(Key::A)) {

            u32 micBuffer_readpos = micBuffer_pos;
            micBuffer_pos = micGetLastSampleOffset();

            while (micBuffer_pos != micBuffer_readpos) {
            soundBuffer[audioBuffer_pos++] = micBuffer[micBuffer_readpos];
            micBuffer_readpos = (micBuffer_readpos + 1) % MIC_BUFFER_SIZE;

            // サイズの送信
            ssize_t sentSize = send(sockfd, &audioBuffer_pos, sizeof(audioBuffer_pos), 0);
            if (sentSize == -1) {
                consoleOfCTRPF.push_back("Failed to send audio data size\n");
            } else {
                consoleOfCTRPF.push_back("Send audio data size success!\n");

                ssize_t sentBytes = send(sockfd, soundBuffer, SOUND_BUFFER_SIZE, 0);
                if (sentBytes == -1) {
                    consoleOfCTRPF.push_back("Failed to send audio data\n");
                } else {
                    consoleOfCTRPF.push_back("Send audio data success!\n");
                }
            }
            }
        }

        if (init && Controller::IsKeyReleased(Key::A)) {
            if (R_FAILED(MICU_StopSampling()))
                consoleOfCTRPF.push_back("Sampling could not be stopped.\n");
            else
                consoleOfCTRPF.push_back("Sampling has been halted.\n");
        }

        if (Controller::IsKeyPressed(Key::B))
            break;

        while (11 < consoleOfCTRPF.size())
            consoleOfCTRPF.erase(consoleOfCTRPF.begin());
        
        screen.DrawRect(30, 20, 340, 200, Color::Black);
        screen.DrawRect(32, 22, 336, 196, Color::White, false);
        for (const auto &log : consoleOfCTRPF)
            screen.DrawSysfont(log, 35, 22 + (&log - &consoleOfCTRPF[0]) * 18, Color::LimeGreen);
        
        OSD::SwapBuffers();        
    }
}

void ProcessReceivedSound(u8* receivedSoundData, u32 receivedSoundSize) {
    // ncsndSound 構造体に受信した音声データをセットする
    svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)soundBuffer, receivedSoundSize);
    ncsndSound receivedSound;
    ncsndInitializeSound(&receivedSound);

    receivedSound.isPhysAddr = true; // 物理アドレス設定
    receivedSound.sampleData = (u8*)svcConvertVAToPA((const void*)receivedSoundData, false); // 受信した音声データをセット
    receivedSound.totalSizeBytes = receivedSoundSize; // 受信した音声データのサイズをセット
    receivedSound.encoding = NCSND_ENCODING_PCM16; // 16ビットPCM
    receivedSound.sampleRate = 16360; // サンプリングレートを設定
    receivedSound.pan = 0.0;
    receivedSound.volume = 1.0;
    // サウンドを再生する
    if (R_FAILED(ncsndPlaySound(0x8, &receivedSound))) {
        console.push_back("Failed to play received sound\n");
    }    
}

bool CloseGameMicHandle(void)
    {
        Handle handles[0x100];
        s32 nbHandles;
            
        nbHandles = svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ALL_HANDLES, (u32)handles, 0);

        for(s32 i = 0; i < nbHandles; i++)
        {
            char name[12];

            if(R_SUCCEEDED(svcControlService(SERVICEOP_GET_NAME, name, handles[i])) && strcmp(name, "mic:u") == 0)
            {
                svcCloseHandle(handles[i]);
                return true;
            }
        }

        return false;
    }
}
