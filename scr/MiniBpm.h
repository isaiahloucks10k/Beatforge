// src/MiniBpm.h
// Adapted from MiniBPM (C) 2012-2025 Particular Programs Ltd
// See mini-bpm-license.txt for terms
#pragma once
#include <vector>

class MiniBPM {
public:
    MiniBPM(float sampleRate);
    ~MiniBPM();

    void setBPMRange(double min, double max);
    double estimateTempoOfSamples(const float* samples, int numSamples);
    std::vector<double> getTempoCandidates() const;
    void reset();

private:
    class Impl;
    Impl* m_impl;
};