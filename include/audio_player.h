// ============================================================
// audio_player.h — I2S MP3 Playback via UDA1334A DAC
// ============================================================
#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>

class AudioPlayer {
public:
    void begin();
    void playAdzan(bool fullVersion = false);    // Play from LittleFS
    void stop();
    void setVolume(int vol);                     // 0–100
    bool isPlaying();
    void loop();                                 // Call in main loop to feed audio buffer

private:
    bool _playing = false;
    int _volume = 80;
};

extern AudioPlayer audioPlayer;

#endif // AUDIO_PLAYER_H
