#include "audiodropwidget.h"
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QDir>
#include <QFileInfo>


static bool isAudioFile(const QString &path) {
    QString lower = path.toLower();
    return lower.endsWith(".mp3") ||
           lower.endsWith(".wav") ||
           lower.endsWith(".flac");
}

static QStringList scanFolderForAudio(const QString &folderPath) {
    QStringList results;
    QDir dir(folderPath);

    QFileInfoList list = dir.entryInfoList(
        QDir::Files | QDir::NoDotAndDotDot | QDir::Dirs,
        QDir::Name
        );

    for (const QFileInfo &info : list) {
        if (info.isDir()) {
            results.append(scanFolderForAudio(info.absoluteFilePath()));
        } else if (info.isFile() && isAudioFile(info.absoluteFilePath()))
            results.append(info.absoluteFilePath());
    }

    return results;
}

AudioDropWidget::AudioDropWidget(QWidget *parent) : QWidget(parent)
{
    setAcceptDrops(true);
    setAttribute(Qt::WA_StyledBackground, true); // make stylesheets apply

    auto *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Drop your audio file here", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    setLayout(layout);
    }

void AudioDropWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return event->ignore();

    for (const QUrl &url : event->mimeData()->urls())
    {
        QString path = url.toLocalFile();
        QFileInfo info(path);
        if (info.isFile() && isAudioFile(path))
            return event->acceptProposedAction();

        if (info.isDir()) {
            QStringList found = scanFolderForAudio(path);
            if (!found.isEmpty())
                return event->acceptProposedAction();
        }
    }
    event->ignore();
}

void AudioDropWidget::dropEvent(QDropEvent *event)
{
    for (const QUrl &url : event->mimeData()->urls())
    {
        QString path = url.toLocalFile();
        QFileInfo info(path);

        if (info.isFile() && isAudioFile(path)) {
            emit audioDropped(path);
        } else if (info.isDir()) {
            QStringList audioFiles = scanFolderForAudio(path);
            for (const QString &file : audioFiles) {
                emit audioDropped(file);
            }
        }
    }
}













