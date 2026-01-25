#pragma once
#include "FFTProcessor.h"
#include <algorithm>
#include <qdebug.h>
#include <qlogging.h>

FFTProcessor::FFTProcessor(int chunkSize, int sampleRate, double& freqStrength)
    : m_chunkSize(chunkSize), m_sampleRate(sampleRate), m_freqStrength(freqStrength)
{
    m_outputBinsSize = chunkSize / 2 + 1;
    m_realInput = fftw_alloc_real(chunkSize);
    m_complexOutput = fftw_alloc_complex(m_outputBinsSize);
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

// - average chunk for results instead of single loudest-peak finding
bool FFTProcessor::isProminentPeak(const std::vector<double>& currentChunk, double targetFrequencyMagnitude, double freqStrength,
    const int& controlNoteBin)
{
    if (targetFrequencyMagnitude == 0) return false;

    double maxVal = *std::max_element(currentChunk.begin(), currentChunk.end());

    return targetFrequencyMagnitude >= freqStrength * maxVal;
}

double FFTProcessor::aWeightLinear(double f) {
    double f2 = f * f;

    double ra =
        (12200.0 * 12200.0 * f2 * f2) /
        ((f2 + 20.6 * 20.6) *
         (f2 + 12200.0 * 12200.0) *
         std::sqrt((f2 + 107.7 * 107.7) *
                   (f2 + 737.9 * 737.9)));

    double AdB = 20.0 * std::log10(ra) + 2.0;
    return std::pow(10.0, AdB / 20.0); // amplitude gain
}

std::vector<double>& FFTProcessor::smoother(std::vector<double>& magnitudes, std::vector<double>& smoothed) {
    // = 1/6 octave
    constexpr double kBandwidth = 0.12;

    double binHz = (double)m_sampleRate / m_chunkSize;

    for (int i = 0; i < m_outputBinsSize; ++i) {
        // double f = m_binFreq[i];
        double f = i * binHz;

        int halfWidthBins = static_cast<int>((f * kBandwidth) / binHz);
        if (halfWidthBins < 1) halfWidthBins = 1;

        double sum = 0.0;
        int count = 0;

        int start = std::max(1, i - halfWidthBins);
        int end = std::min((int)m_outputBinsSize - 1, i + halfWidthBins);

        for (int j = start; j <= end; ++j) {
            sum += magnitudes[j];
            ++count;
        }

        smoothed[i] = sum / count;
    }

    return smoothed;
}

void FFTProcessor::compute(const std::vector<double>& audioData, std::vector<double> targetFrequency, const int productLength,
    const bool& isInterpolate, const int& crossfadeSamples)
{
    m_powerChunks.clear();

    m_sampleStorage.clear();

    if (audioData.size() > static_cast<int>(audioData.size()))
    {
        qDebug() << "Source Audio file too large for type conversion" << "\n" "Recevied size: " << audioData.size() << "\n";
        return;
    }
    int maxChunkSize = static_cast<int>(audioData.size());
    int numChunks = (maxChunkSize + m_chunkSize - 1) / m_chunkSize;

    // this precomputes stuff
    m_binFreq.resize(m_outputBinsSize);
    m_aWeight.resize(m_outputBinsSize);
    for(int i = 0; i < m_outputBinsSize; ++i) {
        double f = (double)i * m_sampleRate / m_outputBinsSize;
        m_binFreq[i] = f;
        m_aWeight[i] = aWeightLinear(f);
    }


    for (int chunk = 0; chunk < numChunks; ++chunk)
    {
        std::fill(m_realInput, m_realInput + m_chunkSize, 0);

        int start = chunk * m_chunkSize;
        int end = std::min(start + m_chunkSize, maxChunkSize);

        std::copy(audioData.begin() + start, audioData.begin() + end, m_realInput);

        fftw_execute(m_plan);

        // changed to 'power' instead of magnitude
        std::vector<double> magnitudes(m_outputBinsSize);
        for (int i = 0; i < m_outputBinsSize; ++i)
        {
            double re = m_complexOutput[i][0];
            double im = m_complexOutput[i][1];

            double power = re*re + im*im;

            power *= m_aWeight[i] * m_aWeight[i];

            magnitudes[i] = power;
        }

        std::vector<double> smoothed(m_outputBinsSize, 0.0);
        smoothed = smoother(magnitudes, smoothed);

        m_powerChunks.push_back(std::move(smoothed));

        for (double freq: targetFrequency)
        {

            // // int controlNoteBin = static_cast<int>(freq * m_sampleRate / m_chunkSize);
            int controlNoteBin = static_cast<int>(freq * m_chunkSize / m_sampleRate);
            if (controlNoteBin >= 0 && controlNoteBin < m_outputBinsSize)
            // qDebug() << "freq: " << freq << "\n";
            // for(auto& controlNoteBin : m_binFreq)
            {
                // qDebug() << "controlNoteBin: " << controlNoteBin << "\n";
                double targetFrequencyMagnitude = m_powerChunks.back()[controlNoteBin];
                if (isProminentPeak(m_powerChunks.back(), targetFrequencyMagnitude, m_freqStrength, controlNoteBin))
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














