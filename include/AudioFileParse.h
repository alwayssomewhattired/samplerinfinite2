#ifndef AUDIOFILEPARSE_H
#define AUDIOFILEPARSE_H
#include <vector>
#include <string>

class AudioFileParse
{
public:
    AudioFileParse();
    ~AudioFileParse();

    std::vector<double> readAudioFileAsMono(const std::string& fileName);
    // void applyHanningWindow();

    bool writeWavFile(const std::vector<double>& samples, const std::string& fileName);
    bool appendWavFile(const std::vector<double>& samples, const std::string& fileName);

    const std::vector<double>& getAudioData() const;

    size_t size() const;

private:
    std::vector<double> m_audioData;
};

#endif // AUDIOFILEPARSE_H
