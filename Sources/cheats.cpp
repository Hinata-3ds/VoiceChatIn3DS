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
#include "Socket.hpp"

namespace CTRPluginFramework 
{
    static Socket            soc;
    std::vector<std::string> console    = {};
    //std::vector<std::string> console = {};

    std::vector<u8> sendData;
    std::vector<u8> receivedData[2];
    int    currentIndex = 0;
  
    Handle sendEvent;
    Handle stopSendEvent;
    Handle restartReceiveEvent;
    Handle exitThreadEvent;

    u32 audioBuffer_pos = 0;
    u32 micBuffer_pos = 0;
    u32 micBuffer_readpos = 0;
    std::string  g_serverIP;
    int          g_port;
    bool   init = true;


    bool  CloseGameMicHandle( void );
    int   InputIPAddressAndPort( std::string& serverIP, int& port );

    void  RecvVoiceThread(void *arg)
    {
        svcCreateEvent(&restartReceiveEvent, RESET_ONESHOT);
        svcClearEvent(restartReceiveEvent);

        Handle handles[] = {exitThreadEvent, restartReceiveEvent};

        while(1)
        {
            Result res;
            static u8 receivedSoundBuffer[4096];
            s32 index;
            if(res = svcWaitSynchronizationN(&index, handles, 2, false, 1) != 0x09401BFE && R_SUCCEEDED(res))
            {
                if(index == 0)
                    break;

                svcClearEvent(restartReceiveEvent);
                currentIndex = (currentIndex + 1) % 2;
                receivedData[currentIndex].clear();
            }

            int received = soc.Receive(receivedSoundBuffer, sizeof(receivedSoundBuffer),0);
            if(received == 1 || received == 0)
                continue;
            else 
                //console.push_back("受信に成功しました！\n");

            receivedData[currentIndex].insert(receivedData[currentIndex].end(), receivedSoundBuffer, receivedSoundBuffer + received);       
        }

        svcCloseHandle(restartReceiveEvent);

        svcExitThread();
    }

    void SendVoiceThread(void *arg) 
    {
        svcCreateEvent(&sendEvent, RESET_ONESHOT);
        svcClearEvent(sendEvent);
        Handle handles[] = {exitThreadEvent, sendEvent};
        
        while (1)
        {
            Result res;
            s32 index;

            if (R_SUCCEEDED(svcWaitSynchronizationN(&index, handles, 2, false, 1))) {
                if (index == 0)
                    break;

                u32 micBuffer_readpos = micBuffer_pos;
                micBuffer_pos = micGetLastSampleOffset();
                soundBuffer[audioBuffer_pos++] = micBuffer[micBuffer_readpos];
                micBuffer_readpos = (micBuffer_readpos + 1) % MIC_BUFFER_SIZE;
                

                int SendByte = soc.Send(soundBuffer, 4096, 0); // sendBufferを送信

                if (SendByte == 1 || SendByte == 0) {
                    continue;
                }
                else{
                    //console.push_back("送信に成功しました！\n");
                    svcSignalEvent(stopSendEvent);
                }
                sendData.insert(sendData.end(), soundBuffer, soundBuffer + SendByte);
            }
        }
        svcCloseHandle(sendEvent);

        svcExitThread();
    }

    void  ParentThread(void *arg)
    {
        Result res  = RL_SUCCESS;
        svcCreateEvent(&exitThreadEvent, RESET_STICKY);
        svcCreateEvent(&stopSendEvent, RESET_STICKY);
        static ThreadEx sendThread(SendVoiceThread, 4096, 0x30, -1);
        static ThreadEx recvThread(RecvVoiceThread, 4096, 0x30, -1);

        sendThread.Start(nullptr);
        recvThread.Start(nullptr);


        if (R_FAILED(ncsndInit(false)))
        {
            console.push_back("サウンドの初期化に失敗しました。\n");
            init = false;
        }
        if (!micBuffer)
            res = svcControlMemoryUnsafe((u32 *)&micBuffer, MIC_BUFFER_ADDR, MIC_BUFFER_SIZE, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        
        if (R_FAILED(res))
        {
            console.push_back("マイクバッファのメモリを割り当てることができませんでした。\n");
            init = false;
        }
        else if (R_FAILED(micInit(micBuffer, MIC_BUFFER_SIZE)))
        {
            console.push_back("マイクの初期化に失敗しました。\n");
            init = false;
        }
        
        if(!soundBuffer)
            res = svcControlMemoryUnsafe((u32 *)&soundBuffer, SOUND_BUFFER_ADDR, SOUND_BUFFER_SIZE, MemOp(MEMOP_ALLOC | MEMREGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        else
            res = RL_SUCCESS;
        
        if(R_FAILED(res) || !soundBuffer)
        {
            console.push_back("サウンドバッファのメモリを割り当てることができませんでした\n");
            init = false;
        }

        Sleep(Milliseconds(500));

        if(init)
            console.push_back("ボイスチャットが開始されました\n");
        console.push_back(" 終了\n");
       
        if(R_FAILED(MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_16360, 0, micGetSampleDataSize(), true)))
            console.push_back("サンプリングを開始することができませんでした。\n");
        else{
            console.push_back("サンプリングを開始しました。\n");
        }

        while(1)
        {   
            if(R_SUCCEEDED(svcWaitSynchronization(stopSendEvent, U64_MAX)))
                svcSignalEvent(restartReceiveEvent);
            std::vector<u8> tempData = receivedData[currentIndex];

            u32 address = reinterpret_cast<u32>(tempData.data());
            u32 size = tempData.size();

            
            ncsndSetVolume(0x8, 1, 0);
            ncsndSetRate(0x8, 16360, 1);
            svcFlushProcessDataCache(CUR_PROCESS_HANDLE, address, size);
            ncsndSound receivedSound;
            ncsndInitializeSound(&receivedSound);

            receivedSound.isPhysAddr = true;
            receivedSound.sampleData = (u8 *)svcConvertVAToPA((const void*)receivedData[currentIndex].data(), false);
            receivedSound.totalSizeBytes = receivedData[currentIndex].size();
            receivedSound.encoding = NCSND_ENCODING_PCM16;
            receivedSound.sampleRate = 16360;
            receivedSound.pan = 0.0;
            receivedSound.volume = 1.0;

            if (R_FAILED(ncsndPlaySound(0x8, &receivedSound)))
                console.push_back("受信した音声データの再生に失敗しました\n");
            else {
                svcSignalEvent(sendEvent);
                
            }
        }
        svcSignalEvent(exitThreadEvent);
        svcExitThread();
    }

    void  voiceChatServer( MenuEntry *entry )
    {
        if (g_serverIP.empty())
            console.push_back("IPアドレスとポートが未入力です。\n");

        if(R_FAILED(soc.createSocket(AF_INET, SOCK_STREAM, 0)))
        {
            console.push_back("ソケットの作成に失敗しました。\n");
        }
        else
            console.push_back("ソケットの作成に成功しました。\n");
        
        
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(g_port);
        serverAddr.sin_addr.s_addr = inet_addr(g_serverIP.c_str());

        if (R_FAILED(soc.bindSocket( (const struct sockaddr *)&serverAddr, sizeof(serverAddr))))
        {
            console.push_back("バインドに失敗しました。\n");
            soc.closeSocket();
            return;
        }
        else
        {
            console.push_back("バインドに成功しました。\n");

            if (R_FAILED(soc.listenSocket(1)))
            {
                console.push_back("リスニングに失敗しました。\n");
                soc.closeSocket();
                return;
            }
            else
            {
                console.push_back("リスニングに成功しました！\n");
                socklen_t serverLen = sizeof(serverAddr);

                if (R_FAILED(soc.acceptConnection((struct sockaddr *)&serverAddr, &serverLen)))
                {
                    console.push_back("クライアントとの接続に失敗しました。\n");
                }
                else
                {   
                    console.push_back("クライアントとの接続に成功しました！\n");
                    const Screen &screen = OSD::GetTopScreen();
                    static ThreadEx ServerThread(ParentThread, 4096, 0x30, -1);
                    ServerThread.Start(nullptr);
                    
                    while(1)
                    {
                        while (11 < console.size())
                            console.erase(console.begin());

                        screen.DrawRect(30, 20, 340, 200, Color::Black);
                        screen.DrawRect(32, 22, 336, 196, Color::Magenta, false);
                        for (const auto &log : console)
                            screen.DrawSysfont(log, 35, 22 + (&log - &console[0]) * 18);

                        OSD::SwapBuffers();
                        if(Controller::IsKeyPressed(Key::B))
                            break;
                    }
                }
            }
        }
    }

    void  voiceChatClient( MenuEntry *entry )
    {
        if (g_serverIP.empty())
        {
            console.push_back("IPアドレスとポートが未入力です。\n");
            return;
        }

        if (R_FAILED(soc.createSocket(AF_INET, SOCK_STREAM, 0)))
        {
            console.push_back("ソケットの作成に失敗しました。\n");
            return;
        }
        console.push_back("ソケットの作成に成功しました！\n");

        struct sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(g_port);
        clientAddr.sin_addr.s_addr = inet_addr(g_serverIP.c_str());

        if (R_FAILED(soc.connectSocket((const struct sockaddr *)&clientAddr, sizeof(clientAddr))))
        {
            console.push_back("サーバーに接続できませんでした。\n");
            return;
        }
        else
            console.push_back("サーバーとの接続に成功しました!\n");

        static ThreadEx ClientThread(ParentThread, 4096, 0x30, -1);
        ClientThread.Start(nullptr);
        const Screen &screen = OSD::GetTopScreen();

        while(1)
        {
            while (11 < console.size())
                console.erase(console.begin());

            screen.DrawRect(30, 20, 340, 200, Color::Black);
            screen.DrawRect(32, 22, 336, 196, Color::Magenta, false);
            for (const auto &log : console)
                screen.DrawSysfont(log, 35, 22 + (&log - &console[0]) * 18);

            OSD::SwapBuffers();

            if(Controller::IsKeyPressed(Key::B))
                break;
        }

    }


    bool  CloseGameMicHandle( void )
    {
        s32    nbHandles;
        Handle handles[0x100];
        
        nbHandles = svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ALL_HANDLES, (u32)handles, 0);

        for ( s32 i = 0; i < nbHandles; i++ )
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

    void InputIPAddressAndPort(MenuEntry *entry) 
    {
        Keyboard kb("[IPアドレス:ポート]を入力してください。");

        std::string ipAndPort;
        kb.IsHexadecimal(true);
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
}