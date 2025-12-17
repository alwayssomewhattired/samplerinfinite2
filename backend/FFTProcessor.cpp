#pragma once
#include "FFTProcessor.h"
#include <algorithm>
#include <qdebug.h>
#include <qlogging.h>

FFTProcessor::FFTProcessor(int chunkSize, int sampleRate, double& freqStrength)
    : m_chunkSize(chunkSize), m_sampleRate(sampleRate), m_freqStrength(freqStrength)
{
    m_fftSize = chunkSize / 2 + 1;
    m_realInput = fftw_alloc_real(chunkSize);
    m_complexOutput = fftw_alloc_complex(m_fftSize);
    m_plan = fftw_plan_dft_r2c_1d(chunkSize, m_realInput, m_complexOutput, FFTW_MEASURE);
}

FFTProcessor::~FFTProcessor()
{
    if (m_plan)
    {
        fftw_destroy_plan(m_plan);
    }
    if (m_realInput)
    {
        fftw_free(m_realInput);
    }
    if (m_complexOutput)
    {
        fftw_free(m_complexOutput);
    }
}

bool FFTProcessor::isProminentPeak(const std::vector<double>& vec, double targetFrequency, double freqStrength)
{
    if (targetFrequency == 0) return false;

    double maxVal = *std::max_element(vec.begin(), vec.end());
    // qDebug() << "target frequency: " << targetFrequency;
    // qDebug() << "current frequency: " << maxVal;
    return targetFrequency >= freqStrength * maxVal;
}
void FFTProcessor::compute(const std::vector<double>& audioData, std::vector<double> targetFrequency, const int productLength,
    const bool& isInterpolate, const int& crossfadeSamples)
{
    m_magnitudeChunks.clear();

     m_sampleStorage.clear();

    if (audioData.size() > static_cast<int>(audioData.size()))
    {
        qDebug() << "Source Audio file too large for type conversion" << "\n" "Recevied size: " << audioData.size() << "\n";
        return;
    }

    int maxChunkSize = static_cast<int>(audioData.size());
    int numChunks = (maxChunkSize + m_chunkSize - 1) / m_chunkSize;

    for (int chunk = 0; chunk < numChunks; ++chunk)
    {
        std::fill(m_realInput, m_realInput + m_chunkSize, 0);

        int start = chunk * m_chunkSize;
        int end = std::min(start + m_chunkSize, maxChunkSize);

        std::copy(audioData.begin() + start, audioData.begin() + end, m_realInput);

        fftw_execute(m_plan);

        std::vector<double> magnitudes(m_fftSize);
        for (int i = 0; i < m_fftSize; ++i)
        {
            magnitudes[i] = std::sqrt(m_complexOutput[i][0] * m_complexOutput[i][0] + m_complexOutput[i][1] * m_complexOutput[i][1]);
        }

        m_magnitudeChunks.push_back(std::move(magnitudes));

        for (double freq: targetFrequency)
        {
            int controlNoteBin = static_cast<int>(freq * m_chunkSize / m_sampleRate);
            if (controlNoteBin >= 0 && controlNoteBin < m_fftSize)
            {
                double targetFrequencyMagnitude = m_magnitudeChunks.back()[controlNoteBin];
                if (isProminentPeak(m_magnitudeChunks.back(), targetFrequencyMagnitude, m_freqStrength))
                {
                    storeChunkIfProminent(audioData, chunk, targetFrequencyMagnitude, freq, productLength, isInterpolate,
                                          crossfadeSamples);
                }
            }
        }
    }
}

const std::vector<double> FFTProcessor::interpolateAudio(const double& beginSample, const double& endSample, const int interpolationNum) {
    std::vector<double> result;
    result.reserve(interpolationNum);

    for (int step = 1; step < interpolationNum; ++step) {
        float t = float(step) / float(interpolationNum + 1);
        float value = beginSample + (endSample - beginSample) * t;
        result.push_back(value);
    }


    return result;
}

void FFTProcessor::storeChunkIfProminent(const std::vector<double>& samples, int counter, double magnitude, double targetFrequency,
                                         const int productLength, const bool& isInterpolate, const int& crossfadeSamples)
{
    qDebug() << "interpolate?: " << isInterpolate << "\n";
    int start = counter * m_chunkSize;
    int end = std::min(start + m_chunkSize, static_cast<int>(samples.size()));
    int targetFrequencyi = std::round(targetFrequency);

    std::vector<double>& m_sampleStorageSpot = m_sampleStorage[targetFrequencyi];
    // length is 8192
    int chunkLen = end - start;

    std::vector<double>& oldChunk = m_previousChunk[targetFrequencyi];
    static std::vector<double> newChunk;

    // crossfade path

    bool isChunkCrossfade = false;
    if (crossfadeSamples > 1)
        isChunkCrossfade = true;

    if (isChunkCrossfade) {
        newChunk.resize(chunkLen);
        std::copy(samples.begin() + start, samples.begin() + end, newChunk.begin());
    }

    // --- crossfade overlapping region ---
    if (isChunkCrossfade && !oldChunk.empty()) {

        for (int i = 0; i < crossfadeSamples; ++i) {
            float t = float(i) / (crossfadeSamples - 1);
            float wA = 1.0f - t;
            float wB = t;

            double blended = std::clamp(
                oldChunk[oldChunk.size() - crossfadeSamples + i] * wA +
                newChunk[i] * wB,
                -1.0, 1.0
                );

            m_sampleStorageSpot.push_back(blended);
        }

        // --- append the remaining new samples
        for (int i = crossfadeSamples; i < chunkLen; ++i) {
            m_sampleStorageSpot.push_back(std::clamp(newChunk[i], -1.0, 1.0));
        }
    }

    if (isChunkCrossfade) {
        oldChunk = newChunk;
        if (m_sampleStorage.count(targetFrequencyi) > 0)
            return;
    }


    // other path

    for (int i = start; i < end; ++i)
    {
        double val = std::clamp(samples[i], -1.0, 1.0);


        m_sampleStorage[targetFrequencyi].push_back(val);


        if (isInterpolate && m_sampleStorage.at(targetFrequencyi).size() > 0) {

            const std::vector interpolatedAudio = FFTProcessor::interpolateAudio(m_sampleStorage[targetFrequencyi].back(), samples.back(), 2);
            for (const double& val : interpolatedAudio)
                m_sampleStorage[targetFrequencyi].push_back(val);
        }
    }

}

const std::vector<std::vector<double>>& FFTProcessor::getMagnitudes() const
{
    return m_magnitudeChunks;
}

std::unordered_map<int, std::vector<double>>& FFTProcessor::getSampleStorage()
{
    return m_sampleStorage;
}














