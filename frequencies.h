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
    const QMap<QString, double>& getterNoteToFreq() const;
    const QMap<double, QString>& getFreqToNote() const;
    const QMap<int, QString>& get_i_freqToNote() const;

private:
    QMap<QString, double> m_noteToFreq;
    QMap<double, QString> m_freqToNote;
    QMap<int, QString> m_i_freqToNote;
    QList<QString> m_notes;
    QList<double> m_freqs;

};

#endif // FREQUENCIES_H
