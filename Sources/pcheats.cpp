// #include "cheats.hpp"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include "csvc.h"
// #include "C:/devkitPro/libctru/include/3ds/services/mic.h"
// #include "Led.hpp"
// #include "ncsnd.h"
// #include <vector>
// #include <fcntl.h>
// #include "Socket.hpp"

// namespace CTRPluginFramework 
// {
//     static Socket            soc;
//     std::vector<std::string> SendConsole    = {};
//     std::vector<std::string> ReceiveConsole = {};

//     std::vector<u8> sendData;
//     std::vector<u8> receivedData[2];
//     int    currentIndex = 0;
//     bool   init = true;
//     Handle sendEvent;
//     Handle stopSendEvent;
//     Handle restartReceiveEvent;
//     Handle exitThreadEvent;

//     u32 audioBuffer_pos = 0;
//     u32 micBuffer_pos = 0;
//     u32 micBuffer_readpos = 0;


//     bool  CloseGameMicHandle( void );
//     void  InputIPAddressAndPort( std::string& serverIP, int& port );

//     void RecvVoiceThread(void *arg)
//     {
//         svcCreateEvent(&restartReceiveEvent, RESET_ONESHOT);
//         svcClearEvent(restartReceiveEvent);

//         Handle handles[] = {exitThreadEvent, restartReceiveEvent};

//         while(1)
//         {
//             Result res;
//             static u8 receivedSoundBuffer[4096];
//             // soc.Receive(&dataSize,sizeof(dataSize), 0);
//             // ReceiveConsole.push_back(Utils::Format("受け取ったデータ:%08lX", dataSize));
//             // static u8 receivedSoundBuffer = new u8[dataSize];
//             s32 index;
//             if(res = svcWaitSynchronizationN(&index, handles, 2, false, 1) != 0x09401BFE && R_SUCCEEDED(res))
//             {
//                 if(index == 0)
//                     break;

//                 svcClearEvent(restartReceiveEvent);
//                 currentIndex = (currentIndex + 1) % 2;
//                 receivedData[currentIndex].clear();
//             }

//             int received = soc.Receive(receivedSoundBuffer, sizeof(receivedSoundBuffer));
//             if(received == 1 || received == 0)
//                 continue;

//             receivedData[currentIndex].insert(receivedData[currentIndex].end(), receivedSoundBuffer, receivedSoundBuffer + received);       
//         }

//         svcCloseHandle(restartReceiveEvent);

//         svcExitThread();
//     }

//     void SendVoiceThread(void *arg)
//     {
//         svcCreateEvent(&sendEvent, RESET_ONESHOT);
//         svcClearEvent(sendEvent);

//         Handle handles[] = {sendEvent, stopSendEvent};
        
//         while(1)
//         {
//             Result res;
//             svcWaitSynchronization(sendEvent, U64_MAX);

//             // サンプリングした音声データの送信処理
//             int dataSize = micBuffer_pos - micBuffer_readpos;
//             if (dataSize > 0) {
//                 sendData.clear(); 
//                 while (audioBuffer_pos != micBuffer_readpos) {
//                     sendData.push_back(micBuffer[micBuffer_readpos]);
//                     micBuffer_readpos = (micBuffer_readpos + 1) % MIC_BUFFER_SIZE;
//                 }
//                 sendData.clear();
//                 sendData.insert(sendData.end(), micBuffer + micBuffer_readpos, micBuffer + micBuffer_pos);

//                 if(R_FAILED(soc.Send(sendData.data(), sendData.size(), 0))) {
//                     SendConsole.push_back("音声データの送信に失敗しました。\n");
//                 } else {
//                     SendConsole.push_back("音声データの送信に成功しました！\n");
//                 }
//             }
//         }
//     }

//     void ParentThread(void *arg)
//     {
//         static ThreadEx sendThread(SendVoiceThread, 4096, 0x30, -1);
//         static ThreadEx recvThread(RecvVoiceThread, 4096, 0x30, -1);

//         sendThread.Start(nullptr);
//         recvThread.Start(nullptr);

//         while(1)
//         {
//             Controller::Update();
//             if(init && Controller::IsKeyPressed(Key::A))
//             {
//                 if(R_FAILED(MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, MICU_SAMPLE_RATE_16360, 0, micGetSampleDataSize(), true)))
//                     SendConsole.push_back("サンプリングを開始することができませんでした。\n");
//                 else{
//                     SendConsole.push_back("サンプリングを開始しました。\n");
//                     svcSignalEvent(sendEvent);
//                 }
//             }
//             if(init && Controller::IsKeyDown(Key::A))
//             {
//                 micBuffer_readpos = micBuffer_pos;
//                 micBuffer_pos= micGetLastSampleOffset();
//             }

//             if(init && Controller::IsKeyReleased(Key::A))
//             {
//                 if(R_FAILED(MICU_StopSampling()))
//                     SendConsole.push_back("サンプリングを終了できませんでした。\n");
//                 else
//                     SendConsole.push_back("サンプリングを終了しました。");
//             }

//             if(Controller::IsKeyPressed(Key::B))
//                 break;
//             ThreadEx::Yield();

//             svcWaitSynchronization(restartReceiveEvent, U64_MAX);

//             ncsndSetVolume(0x8, 1, 0);
//             ncsndSetRate(0x8, 16360, 1);
//             svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)receivedData[currentIndex], 4096);
//             ncsndSound receivedSound;
//             ncsndInitializeSound(&receivedSound);

//             receivedSound.isPhysAddr = true;
//             receivedSound.sampleData = (u8 *)svcConvertVAToPA((const void*)receivedData[currentIndex], false);
//             receivedSound.totalSizeBytes = 4096;
//             receivedSound.encoding = NCSND_ENCODING_PCM16;
//             receivedSound.sampleRate = 16360;
//             receivedSound.pan = 0.0;
//             receivedSound.volume = 1.0;

//             if (R_FAILED(ncsndPlaySound(0x8, &receivedSound)))
//                 ReceiveConsole.push_back("受信した音声データの再生に失敗しました\n");
//             else
//                 ReceiveConsole.push_back("サウンドの再生に成功しました！\n");
            
//             //svcSignalEvent(exitThreadEvent);

//             ThreadEx::Yield();
//         }
//         svcExitThread();
//     }

//     void  voiceChatServer( MenuEntry *entry )
//     {

//         std::string  IPAddress;
//         int          Port;
//         int          Ret;
//         Ret = InputIPAddressAndPort(IPAddress, Port);

//         if (IPAddress.empty() || Ret = 1)
//         {
//             ReceiveConsole.push_back("IPアドレスとポートが未入力です。\n");
//             if (Controller::IsKeyPressed(Key::B))
//                 return;
//         }
        

//         if(R_FAILED(soc.createSocket(AF_INET, SOCK_STREAM, 0)))
//         {
//             ReceiveConsole.push_back("ソケットの作成に失敗しました。\n");
//         }
//         else
//             ReceiveConsole.push_back("ソケットの作成に成功しました。\n");
        
//         struct sockaddr_in serverAddr;
//         memset(&serverAddr, 0, sizeof(serverAddr));
//         serverAddr.sin_family = AF_INET;
//         serverAddr.sin_port = htons(Port);
//         serverAddr.sin_addr.s_addr = inet_addr(IPAddress.c_str());

//         if (R_FAILED(soc.bindSocket( (const struct sockaddr *)&serverAddr, sizeof(serverAddr))))
//         {
//             ReceiveConsole.push_back("バインドに失敗しました。\n");
//             soc.closeSocket();
//             return;
//         }
//         else
//         {
//             ReceiveConsole.push_back("バインドに成功しました。\n");

//             if (R_FAILED(soc.listenSocket(1)))
//             {
//                 ReceiveConsole.push_back("リスニングに失敗しました。\n");
//                 soc.closeSocket();
//                 return;
//             }
//             else
//             {
//                 ReceiveConsole.push_back("リスニングに成功しました！\n");
//                 socklen_t serverLen = sizeof(serverAddr);

//                 if (R_FAILED(soc.acceptConnection((struct sockaddr *)&serverAddr, &serverLen)))
//                 {
//                     ReceiveConsole.push_back("クライアントとの接続に失敗しました。\n");
//                 }
//                 else
//                 {   
//                     ReceiveConsole.push_back("クライアントとの接続に成功しました！\n");
//                     const Screen &screen = OSD::GetTopScreen();
//                     static ThreadEx ServerThread(ParentThread, 4096, 0x30, -1);
//                     ServerThread.Start(nullptr);
                    
//                     while(1)
//                     {
//                         while (11 < ReceiveConsole.size())
//                             ReceiveConsole.erase(ReceiveConsole.begin());

//                         screen.DrawRect(30, 20, 340, 200, Color::Black);
//                         screen.DrawRect(32, 22, 336, 196, Color::Magenta, false);
//                         for (const auto &log : ReceiveConsole)
//                             screen.DrawSysfont(log, 35, 22 + (&log - &ReceiveConsole[0]) * 18);

//                         OSD::SwapBuffers();
//                     }
//                 }
//             }
                
//             const Screen &screen = OSD::GetTopScreen();

//             while(1)
//             {
//                 while (11 < ReceiveConsole.size())
//                     ReceiveConsole.erase(ReceiveConsole.begin());

//                 screen.DrawRect(30, 20, 340, 200, Color::DeepSkyBlue);
//                 screen.DrawRect(32, 22, 336, 196, Color::Magenta, false);
//                 for (const auto &log : ReceiveConsole)
//                     screen.DrawSysfont(log, 35, 22 + (&log - &ReceiveConsole[0]) * 18);

//                 OSD::SwapBuffers();                        
//             }
//         }
//     }

//     void  voiceChatClient( MenuEntry *entry )
//     {
//         std::string  IPAddress;
//         int          Port;
//         int          Ret;
//         Ret = InputIPAddressAndPort(IPAddress, Port);

//         if (IPAddress.empty() || Ret = 1)
//         {
//             SendConsole.push_back("IPアドレスとポートが未入力です。\n");
//             return;
//         }

  
//         Result res  = RL_SUCCESS;

//         if (R_FAILED(soc.createSocket(AF_INET, SOCK_STREAM, 0)))
//         {
//             SendConsole.push_back("ソケットの作成に失敗しました。\n");
//             return;
//         }
//         SendConsole.push_back("ソケットの作成に成功しました！\n");

//         struct sockaddr_in clientAddr;
//         memset(&clientAddr, 0, sizeof(clientAddr));
//         clientAddr.sin_family = AF_INET;
//         clientAddr.sin_port = htons(Port);
//         clientAddr.sin_addr.s_addr = inet_addr(IPAddress.c_str());

//         if (R_FAILED(soc.connectSocket((const struct sockaddr *)&clientAddr, sizeof(clientAddr))))
//         {
//             SendConsole.push_back("サーバーに接続できませんでした。\n");
//             return;
//         }

//         if (R_FAILED(ncsndInit(false)))
//         {
//             SendConsole.push_back("サウンドの初期化に失敗しました。\n");
//             init = false;
//         }
//         if (!micBuffer)
//             res = svcControlMemoryUnsafe((u32 *)&micBuffer, MIC_BUFFER_ADDR, MIC_BUFFER_SIZE, MemOp(MEMOP_ALLOC | MEMOP_REGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
        
//         if (R_FAILED(res))
//         {
//             SendConsole.push_back("マイクバッファのメモリを割り当てることができませんでした。\n");
//             init = false;
//         }
//         else if (R_FAILED(micInit(micBuffer, MIC_BUFFER_SIZE)))
//         {
//             SendConsole.push_back("マイクの初期化に失敗しました。\n");
//             init = false;
//         }
        
//         if(!soundBuffer)
//             res = svcControlMemoryUnsafe((u32 *)&soundBuffer, SOUND_BUFFER_ADDR, SOUND_BUFFER_SIZE, MemOp(MEMOP_ALLOC | MEMREGION_SYSTEM), MemPerm(MEMPERM_READ | MEMPERM_WRITE));
//         else
//             res = RL_SUCCESS;
        
//         if(R_FAILED(res) || !soundBuffer)
//         {
//             SendConsole.push_back("サウンドバッファのメモリを割り当てることができませんでした\n");
//             init = false;
//         }

//         Sleep(Milliseconds(500));

//         if(init)
//             SendConsole.push_back(" 話す\n");
//         SendConsole.push_back(" 終了\n");

//         static ThreadEx ClientThread(ParentThread, 4096, 0x30, -1);
//         ClientThread.Start(nullptr);
//         const Screen &screen = OSD::GetTopScreen();

//         while(1)
//         {
//             while (11 < SendConsole.size())
//                 SendConsole.erase(SendConsole.begin());

//             screen.DrawRect(30, 20, 340, 200, Color::Black);
//             screen.DrawRect(32, 22, 336, 196, Color::Magenta, false);
//             for (const auto &log : SendConsole)
//                 screen.DrawSysfont(log, 35, 22 + (&log - &SendConsole[0]) * 18);

//             OSD::SwapBuffers();    
//         }

//     }


//     bool  CloseGameMicHandle( void )
//     {
//         s32    nbHandles;
//         Handle handles[0x100];
        
//         nbHandles = svcControlProcess(CUR_PROCESS_HANDLE, PROCESSOP_GET_ALL_HANDLES, (u32)handles, 0);

//         for ( s32 i = 0; i < nbHandles; i++ )
//         {
//             char name[12];

//             if(R_SUCCEEDED(svcControlService(SERVICEOP_GET_NAME, name, handles[i])) && strcmp(name, "mic:u") == 0)
//             {
//                 svcCloseHandle(handles[i]);
//                 return true;
//             }
//         }

//         return false;
//     }

//     int InputIPAddressAndPort(std::string& serverIP, int& port)
//     {
//         Keyboard Keyboard("[IPアドレス:ポート番号]を入力してください。");

//         std::string ipAndPort;
//         Keyboard.IsHexadecimal(true);
//         int result = Keyboard.Open(ipAndPort);
//         if (result == -1) {
//             MessageBox("Error occurred")();
//             return 1;
//         }

//         size_t pos = ipAndPort.find(":");
//         if (pos == std::string::npos) {
//             MessageBox("Invaild input format")();
//             return 1;
//         }

//         std::string IPAddress = ipAndPort.substr(0, pos);
//         std::string portStr = ipAndPort.substr(pos + 1);

//         serverIP = IPAddress;

//         try {
//             port = std::stoi(portStr);
//         } catch (const std::exception& exp) {
//             MessageBox("Invaild port number format")();
//             return 1;
//         }

//         MessageBox("IP Address and Port Number saved")();
//     }
// }