#pragma once
#include <QList>
#include <map>
#include "AudioFileParse.h"
#include "FFTProcessor.h"
// #include <onnxruntime_cxx_api.h>


namespace AudioBackend {
	class SamplerInfinite
	{
	public:
        SamplerInfinite();
		~SamplerInfinite();

        void process(const QString& freqs, const std::vector<std::string>& filePaths, const std::map<std::string, double>& freqMap,
            const std::map<double, std::string>& freqToNote, const std::map<int, std::string>& i_freqToNote,
            const bool& isAppend, const bool& isInterpolate, const bool& m_isDemucs, const bool& m_isNonSampled, const int& crossfadeSamples);

        void setFreqStrength(double freqStrength);

        void setOutputDirectory(const std::string& outputDirectory);

	private:
		struct Config {
			int chunkSize;
			int sampleRate;
			int channels;
			int productDurationSamples;
		};
		Config config;

        void runDemucs(const std::vector<std::string>& filePaths);
        AudioFileParse parser;
        FFTProcessor fftProcessor;

        double m_freqStrength{1.0};
        std::string m_outputDirectory;

        // filename to full file-samples vector
        std::unordered_map<std::string, std::vector<double>> sampledInfinites;

        // Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "SamplerInfiniteGUI"};
	};
}
