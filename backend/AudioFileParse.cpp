#define _USE_MATH_DEFINES
#pragma once
#include "AudioFileParse.h"
#include <sndfile.h>
#include <QtLogging>
#include <qDebug>
#include <QFileInfo>

AudioFileParse::AudioFileParse(){}

AudioFileParse::~AudioFileParse(){}

const std::vector<double>& AudioFileParse::getAudioData() const
{
    return m_audioData;
}

size_t AudioFileParse::size() const {
    return m_audioData.size();
}

std::vector<double> AudioFileParse::readAudioFileAsMono(const std::string& fileName)
{
    SF_INFO sfInfo = { 0 };
    SNDFILE* inFile = sf_open(fileName.c_str(), SFM_READ, &sfInfo);

    if (!inFile)
    {
        qDebug() << "libsndfile error: " << sf_strerror(NULL);
        qDebug() << "Error opening file: " << fileName.c_str() << "\n";
        std::remove(fileName.c_str());
        return {};
    }

    size_t numFrames = sfInfo.frames;
    int numChannels = sfInfo.channels;
    qDebug() << numFrames;
    qDebug() << numChannels;
    std::vector<double> rawData(numFrames * numChannels);

    qDebug() << "rawData size: " << rawData.size() << "\n";

    if (sf_read_double(inFile, rawData.data(), rawData.size()) <= 0)
    {
        qDebug() << "Error opening audio data from: " << fileName << "\n";
        sf_close(inFile);
        return {};
    }

    sf_close(inFile);

    if (numChannels == 1)
    {
        return rawData;
    }
    else if (numChannels == 2)
    {
        m_audioData.resize(numFrames);
        for (size_t i = 0; i < numFrames; ++i)
        {
            m_audioData[i] = (rawData[2 * i] + rawData[2 * i + 1]) / 2.0;
        }
        return m_audioData;
    }
    else
    {
        qDebug() << "Unsupported number of channels: " << numChannels << "\n";
    }
}

bool AudioFileParse::writeWavFile(const std::vector<double>& samples, const std::string& fileName)
{
    SF_INFO sf_info = { 0 };
    sf_info.channels = 1;
    sf_info.samplerate = 44100;
    sf_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* file = sf_open(fileName.c_str(), SFM_WRITE, &sf_info);
    if (!file)
    {
        qDebug() << "Error opening file: " << fileName << "\n";
        std::remove(fileName.c_str());
        return false;
    }

    sf_count_t expectedFrames = static_cast<sf_count_t>(samples.size()) / sf_info.channels;
    sf_count_t framesWritten = sf_writef_double(file, samples.data(), expectedFrames);

    if (framesWritten != expectedFrames)
    {
        qDebug() << "Error writing to file: " << fileName << "\n"
                 << "Frames expected: " << expectedFrames
                 << ", frames written: " << framesWritten << "\n";
        std::remove(fileName.c_str());
        return false;
    }

    if (sf_close(file) != 0)
    {
        qDebug() << "Error closing file: " << fileName << "\n";
    }
    qDebug() << "file name!: " << fileName;
    return true;
}

bool AudioFileParse::appendWavFile(const std::vector<double>& samples, const std::string& fileName) {
    SF_INFO sfinfo = { };
    sfinfo.channels = 1;
    sfinfo.samplerate = 44100;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    bool exists = QFileInfo(QString::fromStdString(fileName)).exists();

    SNDFILE* sndfile = nullptr;

    if (exists) {
        sndfile = sf_open(fileName.c_str(), SFM_RDWR, &sfinfo);
        if (!sndfile) {
            qDebug() << "Error opening existing file: " << sf_strerror(NULL) << "\n";
            return false;
        }

        sf_seek(sndfile, 0, SF_SEEK_END);
    } else {

        sndfile = sf_open(fileName.c_str(), SFM_WRITE, &sfinfo);
        if (!sndfile) {
            qDebug() << "Error creating file: " << sf_strerror(NULL) << "\n";
            return false;
        }
    }

    sf_count_t frameCount = samples.size() / sfinfo.channels;
    sf_count_t written = sf_writef_double(sndfile, samples.data(), frameCount);

    if (written != frameCount) {
        qDebug() << "Failed to write all frames: " << sf_strerror(sndfile) << "\n";
        sf_close(sndfile);
        return false;
    }

    sf_close(sndfile);

    return true;
}


// void AudioFileParse::applyHanningWindow()
// {
//     int n = static_cast<int>(m_audioData.size());
//     for (int i = 0; i < n; i++)
//     {
//         double hannValue = 0.5 * (1 - cos(2 * M_PI * i / (n - 1)));
//         m_audioData[i] *= hannValue;
//     }
// }













