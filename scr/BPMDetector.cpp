#include "BPMDetector.h"
#include "MiniBpm.h"
#include <Geode/loader/Log.hpp>
#include <FMOD/fmod.hpp>
#include <FMOD/fmod_errors.h>
#include <cmath>
#include <cstring>

BPMResult BPMDetector::analyzeFile(const std::string& filePath) {
    BPMResult result{};
    
    // Use FMOD to open and decode the audio file
    FMOD::System* fmodSystem = nullptr;
    FMOD::Sound* sound = nullptr;

    // Get GD's existing FMOD system
    // In GD, the FMOD system can be accessed via FMODAudioEngine
    auto* audioEngine = FMODAudioEngine::sharedEngine();
    if (!audioEngine) {
        log::error("Failed to get FMODAudioEngine");
        return result;
    }

    // Get the system from the engine
    FMOD::System* system = audioEngine->m_system;
    if (!system) {
        log::error("Failed to get FMOD system");
        return result;
    }

    // Create the sound with FMOD_OPENONLY so we just decode, don't play
    FMOD_CREATESOUNDEXINFO exinfo;
    memset(&exinfo, 0, sizeof(FMOD_CREATESOUNDEXINFO));
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);

    FMOD_RESULT res = system->createSound(
        filePath.c_str(),
        FMOD_OPENONLY | FMOD_LOWMEM,
        &exinfo,
        &sound
    );

    if (res != FMOD_OK || !sound) {
        log::error("FMOD createSound failed: {} ({})", 
                   FMOD_ErrorString(res), filePath);
        return result;
    }

    // Get format info
    FMOD_SOUND_FORMAT format;
    int channels;
    int bits;
    sound->getFormat(nullptr, &format, &channels, &bits);

    // Get length in PCM samples
    unsigned int lengthBytes = 0;
    sound->getLength(&lengthBytes, FMOD_TIMEUNIT_PCMBYTES);

    unsigned int numSamples = 0;
    sound->getLength(&numSamples, FMOD_TIMEUNIT_PCMSAMPLES);

    if (numSamples == 0) {
        sound->release();
        return result;
    }

    // Determine sample rate from the sound
    float sampleRate = 44100.0f; // default
    unsigned int lengthMS = 0;
    sound->getLength(&lengthMS, FMOD_TIMEUNIT_MS);
    if (lengthMS > 0) {
        sampleRate = (float)numSamples / (lengthMS / 1000.0f);
    }

    // Read the raw PCM data
    char* buffer = new char[lengthBytes];
    unsigned int bytesRead = 0;
    res = sound->readData(buffer, lengthBytes, &bytesRead);
    if (res != FMOD_OK) {
        log::error("FMOD readData failed: {}", FMOD_ErrorString(res));
        delete[] buffer;
        sound->release();
        return result;
    }

    // Convert to mono float samples (-1.0 to 1.0)
    int totalSamples = bytesRead / (bits / 8);
    std::vector<float> monoBuffer;
    monoBuffer.reserve(totalSamples / channels);

    if (bits == 16) {
        short* samples = reinterpret_cast<short*>(buffer);
        int sampleCount = bytesRead / sizeof(short);
        for (int i = 0; i < sampleCount; i += channels) {
            float val = 0.0f;
            for (int c = 0; c < channels; c++) {
                val += samples[i + c] / 32768.0f;
            }
            monoBuffer.push_back(val / channels);
        }
    } else if (bits == 32) {
        // Could be float or 32-bit int
        float* samples = reinterpret_cast<float*>(buffer);
        int sampleCount = bytesRead / sizeof(float);
        for (int i = 0; i < sampleCount; i += channels) {
            float val = 0.0f;
            for (int c = 0; c < channels; c++) {
                val += samples[i + c];
            }
            monoBuffer.push_back(val / channels);
        }
    }

    delete[] buffer;
    sound->release();

    if (monoBuffer.empty()) {
        return result;
    }

    // Run BPM detection
    return analyzePCM(monoBuffer.data(), (int)monoBuffer.size(), (int)sampleRate);
}

BPMResult BPMDetector::analyzePCM(const float* samples, int numSamples, int sampleRate) {
    BPMResult result{};
    result.sampleRate = sampleRate;
    result.durationSeconds = (double)numSamples / sampleRate;

    MiniBPM bpmDetector((float)sampleRate);
    bpmDetector.setBPMRange(60.0, 200.0);
    
    result.bpm = bpmDetector.estimateTempoOfSamples(samples, numSamples);
    
    if (result.bpm <= 0.0) {
        log::warn("BPM detection returned 0 (clip too short or no beats detected)");
        return result;
    }

    result.candidates = bpmDetector.getTempoCandidates();
    
    // Estimate confidence based on candidate spread
    if (result.candidates.size() >= 2) {
        double spread = std::abs(result.candidates[0] - result.candidates[1]);
        result.confidence = std::clamp(1.0 - (spread / result.bpm) * 2.0, 0.0, 1.0);
    } else {
        result.confidence = 0.5;
    }

    log::info("BPM detected: {:.2f} (confidence: {:.2f})", result.bpm, result.confidence);
    return result;
}