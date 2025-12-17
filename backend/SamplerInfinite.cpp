
#include "include/SamplerInfinite.h"
#include "include/FFTProcessor.h"
#include "include/AudioFileParse.h"
#include <qdebug.h>
#include <qlogging.h>
#include <QDir>
#include <QMessageBox>
#include <string>
#include <filesystem>
#include <onnxruntime_cxx_api.h>


Backend::SamplerInfinite::SamplerInfinite()
:     config{
          8192,       // chunkSize
          44100,      // sampleRate
          1,          // channels
          0           // productDurationSamples
      }, fftProcessor(config.chunkSize, config.sampleRate, m_freqStrength)

{

};

Backend::SamplerInfinite::~SamplerInfinite(){

};

void Backend::SamplerInfinite::process(const QString& freqs, const std::vector<std::string>& filePaths, const std::map<std::string, double>& freqMap,
    const std::map<double, std::string>& freqToNote, const std::map<int, std::string>& i_freqToNote,
            const bool& isAppend, const bool& isInterpolate, const bool& isDemucs, const bool& isNonSampled, const int& crossfadeSamples)
{
    qDebug("processing :)\n");

    // if nonsampled true, then juce send file to demucs
    if (isNonSampled) {
        for (auto& song : filePaths) {
            std::vector<double> audio = parser.readAudioFileAsMono(song);
            cudaProcessor(song, audio);
        }

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

    // here we feed out created file into demucs
    // a function that implements ai percussion removal
    // pass audio data from finished sampledinfinite into this function

    if (isDemucs) {
        for (auto& [k, v] : sampledInfinites) {
            cudaProcessor(k, v);
        }
    }

    qDebug("FINISHED!");
}


struct DemucsIO {
    Ort::AllocatedStringPtr inputName;
    Ort::AllocatedStringPtr outputName;
};

static DemucsIO& getDemucsIO(Ort::Session& session) {
    size_t num_inputs = session.GetInputCount();
    for (size_t i = 0; i < num_inputs; ++i) {
        Ort::AllocatorWithDefaultOptions allocator;
        auto name = session.GetInputNameAllocated(i, allocator);

        auto typeInfo = session.GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
        auto shape = typeInfo.GetShape();

        QString shapeStr;
        for (size_t j = 0; j < shape.size(); ++j) {
            shapeStr += QString::number(shape[j]);
            if (j + 1 < shape.size()) shapeStr += ", ";
        }

    }


    static Ort::AllocatorWithDefaultOptions allocator;
    static DemucsIO io {
        session.GetInputNameAllocated(0, allocator),
        session.GetOutputNameAllocated(0, allocator)
    };
    return io;
}

void Backend::SamplerInfinite::cudaProcessor(const std::string& fileName, const std::vector<double>& file) {

    static Ort::SessionOptions session_options = []{
        Ort::SessionOptions opts;
        opts.SetIntraOpNumThreads(1);
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

#ifdef USE_CUDA
        OrtCUDAProviderOptions cuda_options{};
        cuda_options.device_id = 0;
        cuda_options.arena_extend_strategy = 0;
        cuda_options.gpu_mem_limit = SIZE_MAX;
        cuda_options.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
        cuda_options.do_copy_in_default_stream = 1;

        session_options.AppendExecutionProvider_CUDA(cuda_options);
#endif


        return opts;
    }();

    std::wstring model_path = L"..\\..\\..\\models\\htdemucs.ort";
    static Ort::Session session(env, model_path.c_str(), session_options);

    qDebug("model loaded successfully\n");

    std::vector<float> left(file.size());
    for (size_t i = 0; i < file.size(); ++i) {
        left[i] = static_cast<float>(file[i]);
    }
    qDebug() << "files size: " << file.size() << "\n";
    std::vector<float> right = left;


    //logging


    Ort::AllocatorWithDefaultOptions allocator;

    size_t inputCount = session.GetInputCount();
    qDebug() << "Input count:" << inputCount;

    for (size_t i = 0; i < inputCount; i++) {
        auto name = session.GetInputNameAllocated(i, allocator);
        qDebug() << "Input" << i << ":" << name.get();

        auto typeInfo = session.GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        auto shape = tensorInfo.GetShape();

        QString shapeStr = "[";
        for (auto d : shape) {
            shapeStr += QString::number(d) + ",";
        }
        shapeStr += "]";
        qDebug() << "  shape:" << shapeStr;
    }

    size_t outputCount = session.GetOutputCount();
    qDebug() << "Output count:" << outputCount;

    for (size_t i = 0; i < outputCount; i++) {
        auto name = session.GetOutputNameAllocated(i, allocator);
        qDebug() << "Output" << i << ":" << name.get();

        auto typeInfo = session.GetOutputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        auto shape = tensorInfo.GetShape();

        QString shapeStr = "[";
        for (auto d : shape) {
            shapeStr += QString::number(d) + ",";
        }
        shapeStr += "]";
        qDebug() << "  shape:" << shapeStr;
    }



    processorDemucsAudio(left, right, file.size(), session, fileName);

    return;

}

std::vector<float> Backend::SamplerInfinite::runDemucsChunk(Ort::Session& session, const std::vector<float>& stereoInput, int chunkSize) {

    qDebug("run has runned");

    std::vector<int64_t> inputShape = {1, 2, chunkSize};
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo,
        const_cast<float*>(stereoInput.data()),
        stereoInput.size(),
        inputShape.data(),
        inputShape.size()
        );

    DemucsIO& io = getDemucsIO(session);

    static std::vector<float> xState;
    if (xState.empty()) {
        xState.resize(1 * 4 * 4 * 2048 * 336);
        std::fill(xState.begin(), xState.end(), 0.0f);
    }

    std::vector<int64_t> xShape = {1, 4, 4, 2048, 336};

    Ort::Value xTensor = Ort::Value::CreateTensor<float>(
        memoryInfo,
        xState.data(),
        xState.size(),
        xShape.data(),
        xShape.size()
        );

    const char* inputNames[] = { "input", "x" };
    const char* outputNames[] = { "output", "add_67" };

    std::vector<Ort::Value> inputTensors;
    inputTensors.push_back(std::move(inputTensor));  // must use std::move
    inputTensors.push_back(std::move(xTensor));
    qDebug("b4 session.run");

    auto outputTensors = session.Run(
        Ort::RunOptions{nullptr},
        inputNames,
        inputTensors.data(),   // pointer to first element
        inputTensors.size(),   // number of inputs
        outputNames,
        2                     // number of outputs
        );

    qDebug("marker");

    float* xOut = outputTensors[0].GetTensorMutableData<float>();
    float* audioOut = outputTensors[1].GetTensorMutableData<float>();

    std::memcpy(xState.data(), xOut, xState.size() * sizeof(float));

    // gets audio shape
    auto audioInfo = outputTensors[1].GetTensorTypeAndShapeInfo();
    auto shape = audioInfo.GetShape();
    int64_t T = shape[3];

    // a center-crop
    int64_t offset = (T - chunkSize) / 2;

    std::vector<float> result(4 * 2 * chunkSize);

    for (int s = 0; s < 4; ++s) {
        for (int c = 0; c < 2; ++c) {
            float* src = audioOut + s * (s * T) + c * T + offset;
            float* dst = result.data() + s * (2 * chunkSize) + c * chunkSize;

            std::memcpy(dst, src, chunkSize * sizeof(float));
        }
    }

    return result;

}

void Backend::SamplerInfinite::processorDemucsAudio(const std::vector<float>& left, const std::vector<float>& right,
                                                    size_t totalSamples, Ort::Session& session, const std::string& fileName) {

    const int chunkSize = 343980;
    const int hopSize   = chunkSize / 2;

    std::vector<float> stems[4][2];
    for (int s = 0; s < 4; ++s)
        for (int c = 0; c < 2; ++c)
            stems[s][c].assign(totalSamples, 0.0f);

    std::vector<float> input(2 * chunkSize, 0.0f);

    for (int start = 0; start < static_cast<int>(totalSamples); start += hopSize) {

        // fill input (zero-padded)
        std::fill(input.begin(), input.end(), 0.0f);

        int copyLength = std::min(chunkSize, static_cast<int>(totalSamples) - start);
        for (int i = 0; i < copyLength; ++i) {
                input[i]             = left[start + i];
                input[i + chunkSize] = right[start + i];
            }
        qDebug() << "input size: " << input.size() << "\n";
        // run Demucs
        auto output = runDemucsChunk(session, input, chunkSize);
        qDebug("marker");
        float* out = output.data(); // [4][2][chunkSize]

        // rectangular overlap-add
        for (int i = 0; i < copyLength; ++i) {
            int idx = start + i;
            if (idx >= totalSamples) continue;

            for (int s = 0; s < 4; ++s) {
                stems[s][0][idx] += out[s*2*chunkSize + i];
                stems[s][1][idx] += out[s*2*chunkSize + chunkSize + i];
            }
        }
    }







    // write files (unchanged)
    for (int s = 0; s < 4; ++s) {
        std::vector<double> mono(totalSamples);
        for (size_t i = 0; i < totalSamples; ++i)
            mono[i] = 0.5 * (stems[s][0][i] + stems[s][1][i]);


        static int count = 0;
        std::string productName;

        if (fileName.find_last_of('_') == std::string::npos) {
            productName = "noName" + std::to_string(count) + ".wav";
        } else {
            size_t underscorePos = fileName.find_last_of('_');

            std::string base = fileName.substr(0, underscorePos);
            std::string ext = fileName.substr(underscorePos);
            productName = base + std::to_string(count) + ext;
        }
        parser.writeWavFile(mono, productName);
        count++;
    }
}

void Backend::SamplerInfinite::setFreqStrength(double freqStrength) {m_freqStrength = freqStrength;}

void Backend::SamplerInfinite::setOutputDirectory(std::string outputDirectory) {m_outputDirectory = outputDirectory;}








