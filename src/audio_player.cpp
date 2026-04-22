// ============================================================
// audio_player.cpp — I2S MP3 Playback via UDA1334A DAC
// ============================================================
#include "audio_player.h"
#include "config.h"

#include <AudioFileSourceLittleFS.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

AudioPlayer audioPlayer;

static AudioGeneratorMP3*       mp3    = nullptr;
static AudioFileSourceLittleFS* file   = nullptr;
static AudioFileSourceBuffer*   buffer = nullptr;
static AudioOutputI2S*          out    = nullptr;

// Buffer size for file reads — prevents LittleFS stalls from crashing decoder
#define AUDIO_BUFFER_SIZE 2048

void AudioPlayer::begin() {
    out = new AudioOutputI2S();
    out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
    out->SetGain((float)_volume / 100.0f);
    Serial.println("[AUDIO] I2S output initialised (UDA1334A)");
    Serial.printf("[AUDIO] Free heap: %u bytes\n", ESP.getFreeHeap());
}

void AudioPlayer::playAdzan(bool fullVersion) {
    // Stop any current playback
    stop();

    // Recreate I2S output to re-establish DAC connection
    // (handles hot-unplug/replug of UDA1334A)
    if (out) {
        delete out;
    }
    out = new AudioOutputI2S();
    out->SetPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
    out->SetGain((float)_volume / 100.0f);

    const char* path = fullVersion ? ADZAN_FULL_PATH : ADZAN_SHORT_PATH;
    Serial.printf("[AUDIO] Free heap before play: %u bytes\n", ESP.getFreeHeap());

    file = new AudioFileSourceLittleFS(path);
    if (!file->isOpen()) {
        Serial.printf("[AUDIO] Failed to open %s\n", path);
        delete file;
        file = nullptr;
        return;
    }

    // Wrap file source in a buffer to prevent flash read stalls
    buffer = new AudioFileSourceBuffer(file, AUDIO_BUFFER_SIZE);

    mp3 = new AudioGeneratorMP3();

    if (!mp3->begin(buffer, out)) {
        Serial.println("[AUDIO] MP3 begin failed");
        stop();
        return;
    }

    _playing = true;
    Serial.printf("[AUDIO] Playing %s\n", path);
    Serial.printf("[AUDIO] Free heap after start: %u bytes\n", ESP.getFreeHeap());
}

void AudioPlayer::stop() {
    if (mp3) {
        if (mp3->isRunning()) mp3->stop();
        delete mp3;
        mp3 = nullptr;
    }
    if (buffer) {
        delete buffer;
        buffer = nullptr;
    }
    if (file) {
        delete file;
        file = nullptr;
    }
    _playing = false;
}

void AudioPlayer::setVolume(int vol) {
    _volume = constrain(vol, 0, 100);
    if (out) {
        out->SetGain((float)_volume / 100.0f);
    }
}

bool AudioPlayer::isPlaying() {
    return _playing && mp3 && mp3->isRunning();
}

void AudioPlayer::loop() {
    if (_playing && mp3) {
        if (mp3->isRunning()) {
            if (!mp3->loop()) {
                // Playback finished
                stop();
                Serial.println("[AUDIO] Playback complete");
                Serial.printf("[AUDIO] Free heap after stop: %u bytes\n", ESP.getFreeHeap());
            }
        } else {
            stop();
        }
    }
}
