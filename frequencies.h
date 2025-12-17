#ifndef FREQUENCIES_H
#define FREQUENCIES_H

#include <QMap>
#include <QList>
class Frequencies
{
public:
    Frequencies();

    const QList<QString>& getterNotes() const;
    const QList<double>& getterFreqs() const;
    const std::map<std::string, double>& getterNoteToFreq() const;
    const std::map<double, std::string>& getFreqToNote() const;
    const std::map<int, std::string>& get_i_freqToNote() const;

private:
    std::map<std::string, double> m_noteToFreq;
    std::map<double, std::string> m_freqToNote;
    std::map<int, std::string> m_i_freqToNote;
    QList<QString> m_notes;
    QList<double> m_freqs;

};

#endif // FREQUENCIES_H
