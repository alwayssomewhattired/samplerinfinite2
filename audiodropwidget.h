#ifndef AUDIODROPWIDGET_H
#define AUDIODROPWIDGET_H

#include <QWidget>
#include <QString>
#include <QUrl>
#include <QList>

class AudioDropWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AudioDropWidget(QWidget *parent = nullptr);


signals:
    void audioDropped(const QString &filePath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QList<QString> files;
};

#endif // AUDIODROPWIDGET_H
