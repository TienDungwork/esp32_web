#ifndef SPEECH_H
#define SPEECH_H

#include "driver_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// 📢 Enum dùng để phát âm thanh tương ứng
enum SpeechCode {
    Zero = 0,  // phát số 0
    One = 1,   // Phát số 1
    Two = 2,   // Phát số 2
    Three = 3, // Phát số 3
    Four = 4,  // Phát số 4
    Five = 5,  // Phát số 5
    Six = 6,   // Phát số 6
    Seven = 7, // Phát số 7
    Eight = 8, // Phát số 8
    Nine = 9,  // Phát số 9

    A = 10, B = 11, C = 12, D = 13, E = 14, F = 15,
    G = 16, H = 17, I = 18, J = 19, K = 20, L = 21,
    M = 22, N = 23, O = 24, P = 25, Q = 26, R = 27,
    S = 28, T = 29, U = 30, V = 31, W = 32, X = 33,
    Y = 34, Z = 35,

    Space = 36,  // Khoảng trắng
    Clear = 37,  // Xóa

    InvalidQrCode = 100,                      // Mã QR không hợp lệ
    WeighStationBusyPleaseTryAgainLater = 101, // Trạm cân bận
    GoThroughWeighStation = 102,             // Mời đi qua
    CompletedPleaseMove = 103,               // Hoàn thành
    PleaseMoveForward = 104,                 // Mời di chuyển tới
    SystemHasNotYetWeighedPleaseComeBack = 105, // Chưa cân xong
    Done = 106                                // Đã xong
};

void SD_Card_Init(void);
void I2S_Init(void);
void playTracks(String cmd);

#ifdef __cplusplus
}
#endif

#endif  // SPEECH_H
