#include "frequencies.h"
#include <QString>
#include <qdebug.h>

Frequencies::Frequencies() {
    // const std::string noteNames[] = { "C", "C#", "D", "D#", "E", "F",
    //                                  "F#", "G", "G#", "A", "A#", "B" };
    // Use QStringLiteral for better performance (avoids runtime allocation)
    const QStringList noteNames = {
        QStringLiteral("C"),  QStringLiteral("C#"), QStringLiteral("D"),
        QStringLiteral("D#"), QStringLiteral("E"),  QStringLiteral("F"),
        QStringLiteral("F#"), QStringLiteral("G"),  QStringLiteral("G#"),
        QStringLiteral("A"),  QStringLiteral("A#"), QStringLiteral("B")
    };

    const double A4_FREQ = 440.0;
    const int A4_INDEX = 9;
    const int A4_OCTAVE = 4;

    for (int octave = 0; octave <= 8; ++octave)
    {
        for (int i = 0; i < 12; ++i)
        {
            int semitoneOffset = (octave - A4_OCTAVE) * 12 + (i - A4_INDEX);
            double freq = A4_FREQ * std::pow(2.0, semitoneOffset / 12.0);

            QString noteName = noteNames[i] + QString::number(octave);
            m_noteToFreq[noteName] = freq;
            m_freqToNote[freq] = noteName;
            // double r = std::round(freq);
            // qDebug()
            //     << "freq =" << freq
            //     << "round(freq) =" << r
            //     << "cast =" << static_cast<int>(r);

            int key = static_cast<int>(std::round(freq));
            m_i_freqToNote[key] = noteName;
            m_notes.append(noteName);
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

const QMap<QString, double>& Frequencies::getterNoteToFreq() const
{
    return m_noteToFreq;
}


const QMap<double, QString>& Frequencies::getFreqToNote() const { return m_freqToNote; }

const QMap<int, QString>& Frequencies::get_i_freqToNote() const { return m_i_freqToNote; }
