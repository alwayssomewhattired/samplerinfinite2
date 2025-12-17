#ifndef FFTPROCESSOR_H
#define FFTPROCESSOR_H
#include <vector>
#include <unordered_map>
#include <fftw3.h>

class FFTProcessor
{
public:

    FFTProcessor(int chunkSize, int sampleRate, double& freqStrength);

    ~FFTProcessor();

    /**
     * @param resetSamples clears the m_sampleStorage
    */
    void compute(const std::vector<double>& audioData, std::vector<double> targetFrequency, const int productLength, const bool& isInterpolate,
            const int& crossfadeSamples);

    const std::vector<std::vector<double>>& getMagnitudes() const;

    std::unordered_map<int, std::vector<double>>& getSampleStorage();

private:

    /**
    * @param ratio ***NEEDS CONTROL***
    */
    bool isProminentPeak(const std::vector<double>& vec, double targetFrequency, double freqStrength);
    const std::vector<double> interpolateAudio(const double& beginSample, const double& endSample, const int interpolationNum);

    void storeChunkIfProminent(const std::vector<double>& samples, int counter, double magnitude, double targetFrequency, const int productLength,
        const bool& isInterpolate, const int& crossfadeSamples);

    int m_chunkSize;
    int m_sampleRate;
    int m_fftSize;

    // points to fftw_alloc_real
    double* m_realInput;

    fftw_complex* m_complexOutput;
    fftw_plan m_plan;

    std::vector<std::vector<double>> m_magnitudeChunks;
    std::unordered_map<int, std::vector<double>> m_sampleStorage;

    double& m_freqStrength;

    std::unordered_map<int, std::vector<double>> m_previousChunk;


};
#endif // FFTPROCESSOR_H
