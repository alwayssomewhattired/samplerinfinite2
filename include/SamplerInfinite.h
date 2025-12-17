#pragma once
#include <QList>
#include <map>
#include "AudioFileParse.h"
#include "FFTProcessor.h"
#include <onnxruntime_cxx_api.h>


namespace Backend {
	class SamplerInfinite
	{
	public:
        SamplerInfinite();
		~SamplerInfinite();

        void process(const QString& freqs, const std::vector<std::string>& filePaths, const std::map<std::string, double>& freqMap,
            const std::map<double, std::string>& freqToNote, const std::map<int, std::string>& i_freqToNote,
            const bool& isAppend, const bool& isInterpolate, const bool& m_isDemucs, const bool& m_isNonSampled, const int& crossfadeSamples);

        void setFreqStrength(double freqStrength);

        void setOutputDirectory(std::string outputDirectory);

	private:
		struct Config {
			int chunkSize;
			int sampleRate;
			int channels;
			int productDurationSamples;
		};
		Config config;

        void cudaProcessor(const std::string& fileName, const std::vector<double>& file);
        std::vector<float> runDemucsChunk(Ort::Session& session, const std::vector<float>& stereoInput, int chunkSize);
        void processorDemucsAudio(const std::vector<float>& left, const std::vector<float>& right,
                                  size_t totalSamples, Ort::Session& session, const std::string& fileName);
        AudioFileParse parser;
        FFTProcessor fftProcessor;

        double m_freqStrength{1.0};
        std::string m_outputDirectory;

        // filename to full file-samples vector
        std::unordered_map<std::string, std::vector<double>> sampledInfinites;

        Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "SamplerInfiniteGUI"};
	};
}
