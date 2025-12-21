
#include "include/SamplerInfinite.h"
#include "include/FFTProcessor.h"
#include "include/AudioFileParse.h"
#include <qdebug.h>
#include <qlogging.h>
#include <QDir>
#include <QProcess>
#include <QMessageBox>
#include <QCoreApplication>
#include <string>
#include <filesystem>


AudioBackend::SamplerInfinite::SamplerInfinite()
:     config{
          8192,       // chunkSize
          44100,      // sampleRate
          1,          // channels
          0           // productDurationSamples
      }, fftProcessor(config.chunkSize, config.sampleRate, m_freqStrength)

{

};

AudioBackend::SamplerInfinite::~SamplerInfinite(){

};

void AudioBackend::SamplerInfinite::runDemucs(const std::vector<std::string>& filePaths) {
    QString baseDir = QCoreApplication::applicationDirPath();
    QDir dir(baseDir);

    QString pythonPath = dir.filePath("../demucs/venv/Scripts/python.exe");
    QString scriptPath = dir.filePath("../demucs/demucsBuild.py");

    QProcess process;
    QStringList arguments;
    arguments << scriptPath;

    if (filePaths.size() == 0) {
        for (auto& [k, v] : sampledInfinites) {
            arguments << QString::fromStdString(k);
        }
    } else {
        for (auto& name : filePaths) {
            std::filesystem::path p(name);
            auto songName = p.stem().string();
            qDebug() << songName;
            arguments << QString::fromStdString(songName);
        }
    }

    process.start(pythonPath, arguments);

    if (!process.waitForStarted()) {
        qDebug() << "Failed to start demucs script";
        qDebug() << "QProcess error:" << process.error();
        qDebug() << "Error string:" << process.errorString();
        return;
    }

    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    qDebug() << "output: " << output;
    qDebug() << "error: " << error;
}

void AudioBackend::SamplerInfinite::process(const QString& freqs, const std::vector<std::string>& filePaths, const std::map<std::string,
            double>& freqMap, const std::map<double, std::string>& freqToNote, const std::map<int, std::string>& i_freqToNote,
            const bool& isAppend, const bool& isInterpolate, const bool& isDemucs, const bool& isNonSampled, const int& crossfadeSamples)
{
    qDebug("processing :)\n");

    for(auto& thingName : filePaths)
        qDebug() << thingName;

    // if nonsampled true, then juce send file to demucs
    if (isNonSampled) {
        runDemucs(filePaths);

        qDebug("FINISHED!");
        return;
    }


    // frequency splitting logic in here... move to a utils at some point
    std::string freqsStr = freqs.toStdString();
    std::vector<double> parts;
    std::string::size_type start = 0;
    std::string::size_type end;

    while ((end = freqsStr.find("\n\n", start)) != std::string::npos)
    {

        std::string slicedFreq = freqsStr.substr(start, end - start);
        parts.push_back(freqMap.at(slicedFreq));

        start = end + 2;

    }

    std::string slicedFreq = freqsStr.substr(start);
    if (!slicedFreq.empty())
        parts.push_back(freqMap.at(slicedFreq));


    // 1. each song
    for (const std::string& song : filePaths)
    {
        std::filesystem::path p(song);
        auto songName = p.stem().string();

        if (isAppend)
            songName = "appendage";

        FFTProcessor fftProcessor(config.chunkSize, config.sampleRate, m_freqStrength);

        parser.readAudioFileAsMono(song);

        int n = parser.size();

        int num_chunks = (n + config.chunkSize - 1) / config.chunkSize;

        // this window is pointless vvv possibly make a control for it... possibly eat mushroom and think
        // parser.applyHanningWindow();

        fftProcessor.compute(parser.getAudioData(), parts, config.productDurationSamples, isInterpolate, crossfadeSamples);

        // wtf do i do with you??? vvv
        const auto& chunks = fftProcessor.getMagnitudes();

        // wtf do i do with you??? vvv
        const std::vector<double>& audioCopy = parser.getAudioData();

        int i = 0;

        for (auto& [k, processedSamples] : fftProcessor.getSampleStorage()) {


            // std::string freq = freqToNote.at(parts[i]);
            std::string freq = i_freqToNote.at(k);
            // this inner loop is terrible. could easily mismatch frequency to samples vvv
            // std::filesystem::path dirPath = m_outputDirectory + '/' + freq;
            std::filesystem::path dirPath = m_outputDirectory + '/' + freq;
            std::filesystem::create_directory(dirPath);
            // make a control that chooses an existing audio file to append new audio to
            // for now, make the choice automatically the 'appendage'.wav
            std::string finalProductName = m_outputDirectory + '/' + freq + "/" + songName + "ohyah" + '_' + freq + ".wav";
            qDebug() << "final product name : " << finalProductName << "\n";
            qDebug() << "freq : " << freq << "\n";
            if (isAppend) {
                parser.appendWavFile(processedSamples, finalProductName);
                auto& dest = sampledInfinites[finalProductName];
                dest.insert(dest.end(), processedSamples.begin(), processedSamples.end());
            }
            else
                parser.writeWavFile(processedSamples, finalProductName);
                sampledInfinites[finalProductName] = processedSamples;
            i++;
        }
    }

    runDemucs({});

    qDebug("FINISHED!");
}

void AudioBackend::SamplerInfinite::setFreqStrength(double freqStrength) {m_freqStrength = freqStrength;}

void AudioBackend::SamplerInfinite::setOutputDirectory(std::string outputDirectory) {m_outputDirectory = outputDirectory;}








