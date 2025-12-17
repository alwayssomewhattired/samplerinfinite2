#include "frequencies.h"
#include <QString>

Frequencies::Frequencies() {
    const std::string noteNames[] = { "C", "C#", "D", "D#", "E", "F",
                                     "F#", "G", "G#", "A", "A#", "B" };

    const double A4_FREQ = 440.0;
    const int A4_INDEX = 9;
    const int A4_OCTAVE = 4;

    for (int octave = 0; octave <= 8; ++octave)
    {
        for (int i = 0; i < 12; ++i)
        {
            int semitoneOffset = (octave - A4_OCTAVE) * 12 + (i - A4_INDEX);
            double freq = A4_FREQ * std::pow(2.0, semitoneOffset / 12.0);

            std::string noteName = noteNames[i] + std::to_string(octave);
            m_noteToFreq[noteName] = freq;
            m_freqToNote[freq] = noteName;
            m_i_freqToNote[std::round(freq)] = noteName;
            m_notes.append(QString::fromStdString(noteName));
            m_freqs.append(freq);
        }
    }
}

const QList<QString>& Frequencies::getterNotes() const
{
    return m_notes;
}

const QList<double>& Frequencies::getterFreqs() const
{
    return m_freqs;
}

const std::map<std::string, double>& Frequencies::getterNoteToFreq() const
{
    return m_noteToFreq;
}


const std::map<double, std::string>& Frequencies::getFreqToNote() const { return m_freqToNote; }

const std::map<int, std::string>& Frequencies::get_i_freqToNote() const { return m_i_freqToNote; }
