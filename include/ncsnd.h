/**
 * @file ncsnd.h
 * @brief Nintendo 3DS用のCSNDインターフェース。
 */
#pragma once

#include <3ds/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/// CSNDハンドル。
extern Handle ncsndCSNDHandle;

/// CSNDチャンネルの最大数。
#define NCSND_NUM_CHANNELS 32

/// ダイレクトサウンドの最大ボリューム。
#define NCSND_DIRECTSOUND_MAX_VOLUME 32768

/// ダイレクトサウンドの出力モード。
typedef enum
{
    NCSND_SOUNDOUTPUT_MONO = 0, ///< モノラル出力
    NCSND_SOUNDOUTPUT_STEREO = 1 ///< ステレオ出力
} ncsndSoundOutputMode;

/// サンプルエンコーディングの可能な形式。
typedef enum
{
    NCSND_ENCODING_PCM8 = 0, ///< PCM8
    NCSND_ENCODING_PCM16,    ///< PCM16
    NCSND_ENCODING_ADPCM,    ///< IMA-ADPCM
} ncsndSoundFormat;

/// ADPCMコンテキスト。
typedef struct
{
    u16 data;   ///< サンプル
    u8 tableIndex; ///< テーブル + インデックス
    u8 padding;
} ncsndADPCMContext;

/// ダイレクトサウンドのチャンネル関連データ。
typedef struct 
{
    u8 channelAmount; ///< 使用されるチャンネルの数。1か2。
    u8 channelEncoding; ///< サンプルのエンコーディング。
    bool isLeftPhys; ///< 左側のサンプルデータが物理アドレスかどうか（不明なら無視）。
    bool isRightPhys; ///< 右側のサンプルデータが物理アドレスかどうか（不明なら無視）。
    u32 sampleRate; ///< 両方のチャンネルのサンプルレート。
    void* leftSampleData; ///< 左耳またはモノラルサンプルデータへのポインタ。
    void* rightSampleData; ///< 右耳またはモノラルサンプルデータへのポインタ。
    u32 sampleDataLength; ///< 個々のサンプルデータバッファのサイズ（バイト単位）。
    ncsndADPCMContext leftAdpcmContext; ///< 左耳またはモノラルチャンネルのIMA ADPCMコンテキスト。
    ncsndADPCMContext rightAdpcmContext; ///< 右耳チャンネルのIMA ADPCMコンテキスト。
} ncsndDirectSoundChannelData;

/// ダイレクトサウンドに適用される修飾子。
typedef struct
{
    float speedMultiplier; ///< サウンド再生速度、デフォルト：1。
    s32 channelVolumes[2]; ///< 各チャンネルのボリューム、最大：32768。
    u8 unknown0; ///< 不明
    u8 padding[3]; ///< パディング？未使用。
    float unknown1; ///< 不明、1に設定されることが多い。再生遅延の種類？
    u32 unknown2; ///< 不明、unknown1に関連しているかもしれない。再生遅延の種類？
    u8 ignoreVolumeSlider; ///< ボリュームスライダーを無視して最大ボリュームで再生するために1に設定。
    u8 forceSpeakerOutput; ///< ヘッドフォンが接続されていてもスピーカーで再生するために1に設定。
    u8 playOnSleep; ///< スリープ時にサウンドを一時停止する場合は0、再生を継続する場合は1に設定。
    u8 padding1; ///< パディング？未使用。
} ncsndDirectSoundModifiers;

/// ダイレクトサウンド構造体。
typedef struct
{
    u8 always0; ///< アプレットによって常に0に設定される。
    u8 soundOutputMode; ///< 出力モード（ncsndSoundOutputMode）。
    u8 padding[2]; ///< パディング？未使用。
    ncsndDirectSoundChannelData channelData; ///< チャンネル関連データ。
    ncsndDirectSoundModifiers soundModifiers; ///< サウンド再生に適用される修飾子。
} ncsndDirectSound;

/// サウンド構造体。
typedef struct
{
    bool isPhysAddr; ///< sampleDataとloopSampleDataが物理アドレスを保持しているかどうか（不明なら無視）。
    void* sampleData; ///< サンプルデータへのポインタ。
    void* loopSampleData; ///< ループポイントサンプルへのポインタ（ループが未使用の場合はsampleDataに設定）。
    u32 totalSizeBytes; ///< 全体のサイズ（バイト単位）。

    ncsndSoundFormat encoding; ///< サンプルデータのエンコーディング。
    bool loopPlayback; ///< 再生をループするかどうか（ループポイントにloopSampleDataを使用）。
    ncsndADPCMContext context; ///< ADPCMコンテキスト。
    ncsndADPCMContext loopContext; ///< ループポイントでのADPCMコンテキスト（ループが未使用の場合はcontextに設定）。

    u32 sampleRate; ///< サウンドのサンプルレート。
    float volume; ///< サウンドのボリューム。
    float pitch; ///< サウンドのピッチ。
    float pan; ///< サウンドのパンニング。
    bool linearInterpolation; ///< 線形補間を有効または無効にするか。
} ncsndSound;

///< 使用が許可されているチャンネルのビットマスク。
extern u32 ncsndChannels;

/**
 * @brief ncsndインターフェースを初期化し、CSNDも初期化します。
 * @param installAptHook APT通知を自動的に処理するかどうか。
 * @return 操作の結果。
 * 
 * APT通知は通常のアプリケーションでのみ正常に機能します。サービス、アプレット、プラグインでは手動で処理してください。
*/
Result ncsndInit(bool installAptHook);

/**
 * @brief APTイベントを通知します。ncsndInitにfalseが渡された場合にのみ必要です。
 * @param event 通知するAPTイベント。APT_HookType列挙型を確認してください。
 * 
 * サービス、アプレット、プラグインではAPT通知の手動処理が必要です。
*/
void ncsndNotifyAptEvent(APT_HookType event);

/**
 * @brief ncsndインターフェースを終了し、CSNDも終了します。
*/
void ncsndExit(void);

/**
 * @brief ダイレクトサウンドをデフォルト値で初期化します。
 * @param sound 初期化するダイレクトサウンド。
*/
void ncsndInitializeDirectSound(ncsndDirectSound* sound);

/**
 * @brief ダイレクトサウンドを再生します。
 * @param chn 使用するダイレクトサウンドチャンネル。範囲[0, 3]。
 * @param priority ダイレクトサウンドの優先度。チャンネルがすでに再生中の場合に使用されます。小さい値ほど優先度が高い。範囲[0, 31]。
 * @param sound 再生するダイレクトサウンド構造体へのポインタ。
 * @return 操作の結果。
*/
Result ncsndPlayDirectSound(u32 chn, u32 priority, ncsndDirectSound* sound);

/**
 * @brief サウンドをデフォルト値で初期化します。
 * @param sound 初期化するサウンド。
*/
void ncsndInitializeSound(ncsndSound* sound);

/**
 * @brief サウンドを再生します。
 * @param chn 使用するサウンドチャンネル。ncsndChannelsで使用可能かどうかを確認してください。
 * @param sound 再生するサウンド構造体へのポインタ。
 * @return 操作の結果。
*/
Result ncsndPlaySound(u32 chn, ncsndSound* sound);

/**
 * @brief チャンネルを停止します。
 * @param chn 停止するサウンドチャンネル。
*/
void ncsndStopSound(u32 chn);

/**
 * @brief チャンネルのボリュームを設定します。
 * @param chn 変更するサウンドチャンネル。
 * @param volume ボリューム値。範囲[0, 1]。
 * @param pan パンニング値。範囲[-1, 1]。
*/
void ncsndSetVolume(u32 chn, float volume, float pan);

/**
 * @brief チャンネルのサンプルレートを設定します。
 * @param chn 変更するサウンドチャンネル。
 * @param volume サンプルレート値。
 * @param pan ピッチ値。範囲[0, 1]。
*/
void ncsndSetRate(u32 chn, u32 sampleRate, float pitch);

/**
 * @brief チャンネルの再生状態を確認します。
 * @param chn 確認するサウンドチャンネル。
*/
bool ncsndIsPlaying(u32 chn);

#ifdef __cplusplus
}

#endif