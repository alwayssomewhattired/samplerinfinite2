
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

void AudioBackend::SamplerInfinite::runDemucs(const std::vector<std::filesystem::path>& filePaths) {
    QDir dir(QCoreApplication::applicationDirPath());

    dir.cdUp(); // Debug
    dir.cdUp(); // diy_msvc-Debug
    dir.cdUp(); // build

    QString pythonPath = dir.filePath("demucs/venv/Scripts/python.exe");
    QString scriptPath = dir.filePath("demucs/demucsBuild.py");

    QProcess process;
    QStringList arguments;
    arguments << scriptPath;
    arguments << m_outputDirectory;

    if (filePaths.size() == 0) {
        for (auto& [k, v] : m_sampledInfinites) {
            arguments << QString::fromStdString(k);
        }
    } else {
        for (auto& name : filePaths) {
            QString path = QString::fromStdWString(name.wstring());
            arguments << path;
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

void AudioBackend::SamplerInfinite::process(const QString& freqs, const std::vector<std::filesystem::path>& paths, const std::map<std::string,
            double>& freqMap, const std::map<double, std::string>& freqToNote, const std::map<int, std::string>& i_freqToNote,
            const bool& isAppend, const bool& isInterpolate, const bool& isDemucs, const bool& isNonSampled, const int& crossfadeSamples)
{
    qDebug("processing :)\n");

    // if nonsampled true, then juce send file to demucs
    if (isNonSampled) {
        runDemucs(paths);
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
    for (const std::filesystem::path& song : paths)
    {
        // std::filesystem::path p(song);
        auto songName = song.stem().string();

        if (isAppend)
            songName = "appendage";

        FFTProcessor fftProcessor(config.chunkSize, config.sampleRate, m_freqStrength);
        parser.readAudioFileAsMono(song);

        int n = parser.size();
        int num_chunks = (n + config.chunkSize - 1) / config.chunkSize;

        // parser.applyHanningWindow();

        fftProcessor.compute(parser.getAudioData(), parts, config.productDurationSamples, isInterpolate, crossfadeSamples);

        // wtf do i do with you??? vvv
        const auto& chunks = fftProcessor.getMagnitudes();

        // wtf do i do with you??? vvv
        const std::vector<double>& audioCopy = parser.getAudioData();

        int i = 0;

        for (auto& [k, processedSamples] : fftProcessor.getSampleStorage()) {

            std::string freq;
            auto it = i_freqToNote.find(k);
            if (it == i_freqToNote.end()) {
                qDebug() << "Missing key:" << k;
                std::terminate();
            } else {
                freq = it->second;
            }
            // this inner loop is terrible. could easily mismatch frequency to samples vvv
            qDebug("fuck");
            std::filesystem::path dirPath = m_outputDirectory.toStdString() +
                                            '/' +  "sampledinfinites" + '/' + freq + "/" + songName + "ohyah" + '_' + freq + ".wav";
            std::filesystem::create_directories(dirPath.parent_path());
            qDebug("you");

            // make a control that chooses an existing audio file to append new audio to
            // for now, make the choice automatically the 'appendage'.wav
            std::string finalProductName = m_outputDirectory.toStdString() +
                                           '/' +  "sampledinfinites" + '/' + freq + "/" + songName + "ohyah" + '_' + freq + ".wav";
            qDebug() << "final product name : " << finalProductName << "\n";
            qDebug() << "freq : " << freq << "\n";
            if (isAppend) {
                parser.appendWavFile(processedSamples, finalProductName);
                auto& dest = m_sampledInfinites[finalProductName];
                dest.insert(dest.end(), processedSamples.begin(), processedSamples.end());
            }
            else
                parser.writeWavFile(processedSamples, finalProductName);
                m_sampledInfinites[finalProductName] = processedSamples;
            i++;
        }
    }
    if (isDemucs)
        runDemucs({});

    qDebug("FINISHED!");
}

void AudioBackend::SamplerInfinite::setFreqStrength(double freqStrength) {m_freqStrength = freqStrength;}

void AudioBackend::SamplerInfinite::setOutputDirectory(QString outputDirectory) {m_outputDirectory = outputDirectory;}








