#pragma once
#include <string>

struct BPMResult {
    double bpm;
    double confidence;         // 0.0 - 1.0
    std::vector<double> candidates;
    double durationSeconds;
    int sampleRate;
};

class BPMDetector {
public:
    /// Analyze a sound file at the given path and return BPM estimate
    static BPMResult analyzeFile(const std::string& filePath);
    
    /// Analyze raw PCM float samples (mono, -1.0 to 1.0)
    static BPMResult analyzePCM(const float* samples, int numSamples, int sampleRate);
};