#include "samplerinfinite.h"
#include "./ui_samplerinfinite.h"
#include "audiodropwidget.h"
#include "poweroftwospinbox.h"
#include "include/SamplerInfinite.h"
#include <QScrollBar>
#include <QFileDialog>


SamplerInfinite::SamplerInfinite(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SamplerInfinite)
{
    ui->setupUi(this);
    this->showMaximized();

    ui->fileBrowser->setReadOnly(true);
    ui->fileBrowser->setFrameShape(QFrame::NoFrame);


    auto *dropWidget = ui->widget;

    connect(dropWidget, &AudioDropWidget::audioDropped, this, [&](const QString &filePath){
        qDebug() << "file dropped: " << filePath;
        QTextCharFormat format;
        format.setForeground(Qt::green);

        QTextCursor cursor(ui->fileBrowser->textCursor());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(filePath + "\n" + "\n", format);

        qDebug() << "filePath: " << filePath;

        filePaths.push_back(filePath);
        ui->fileBrowser->verticalScrollBar()->setValue(ui->fileBrowser->verticalScrollBar()->maximum());
    });

    auto *freqDisplayWidget = ui->freqDisplay;
    auto *searchableComboBox = ui->searchablecombobox;

    connect(searchableComboBox, &SearchableComboBox::itemSelected, this, [this, freqDisplayWidget](const QString &text){
        freqDisplayWidget->setReadOnly(true);


        QTextCharFormat format;
        format.setForeground(Qt::green);

        QTextCursor cursor(freqDisplayWidget->textCursor());
        cursor.movePosition(QTextCursor::End);
        if(text == "all") {
            for(const QString& note : m_frequencies.getterNotes()) {
                cursor.insertText(note + "\n" + "\n", format);
                cursor.movePosition(QTextCursor::End);
            }
        } else {
            cursor.insertText(text + "\n" + "\n", format);
        }
        freqDisplayWidget->verticalScrollBar()->setValue(freqDisplayWidget->verticalScrollBar()->maximum());
    });

    auto *startButton = ui->startButton;

    connect(startButton, &QPushButton::clicked, this, [this, freqDisplayWidget](){
        // conversions
        std::map<std::string, double> stdMap;
        auto& qMap = m_frequencies.getterNoteToFreq();
        for (auto it = qMap.constBegin(); it != qMap.constEnd(); ++it) {
            stdMap.insert({it.key().toStdString(), it.value()});
        }

        std::map<double, std::string> freqToNote;
        auto& qFreqToNote = m_frequencies.getFreqToNote();
        for (auto it = qFreqToNote.constBegin(); it != qFreqToNote.constEnd(); ++it) {
            freqToNote.insert({it.key(), it.value().toStdString()});
        }

        std::map<int, std::string> iFreqToNote;
        auto& qIFreqToNote = m_frequencies.get_i_freqToNote();
        for (auto it = qIFreqToNote.constBegin(); it != qIFreqToNote.constEnd(); ++it) {
            iFreqToNote.insert({it.key(), it.value().toStdString()});
        }

        const std::vector<QString> localPaths = filePaths;

        std::vector<std::filesystem::path> encodedPaths;
        encodedPaths.reserve(localPaths.size());

        for (const QString& file : localPaths) {
            QByteArray utf8 = file.toUtf8();
            encodedPaths.emplace_back(std::string(utf8.constData(), utf8.size()));
        }

        for (const auto& p : encodedPaths) {
            const auto& u8 = p.u8string();
            qDebug() << QString::fromUtf8(
                reinterpret_cast<const char*>(u8.data()),
                static_cast<int>(u8.size())
                );
        }


        m_backend.process(freqDisplayWidget->toPlainText(), encodedPaths, stdMap, freqToNote,
        iFreqToNote, m_isAppend, m_isInterpolate, m_isDemucs, m_isNonSampled, m_chunkCrossfadeSamples);
    });

    auto* freqStrengthSlider = ui->freqStrengthSlider;

    connect(freqStrengthSlider, &QSlider::sliderReleased, this, [this, freqStrengthSlider](){
        m_backend.setFreqStrength(freqStrengthSlider->value() / 100.0);
    });

    auto* outputDirectoryButton = ui->OutputDirectoryButton;
    auto* outputDirectoryLabel = ui->OutputDirectoryLabel;

    connect(outputDirectoryButton, &QPushButton::clicked, this, [this, outputDirectoryLabel](){
        QString dir = QFileDialog::getExistingDirectory(
            this,
            tr("Select Directory"),
            QDir::homePath(),
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );

        if (!dir.isEmpty()) {
            qDebug() << "Selected directory: " << dir;
            outputDirectoryLabel->setText(dir);
            m_backend.setOutputDirectory(dir);
        }

    });

    auto* appendModeButton = ui->AppendModeButton;
    appendModeButton->setStyleSheet(
        "QPushButton { background-color: rgb(0, 85, 0); }"
        "QPushButton::checked { background-color: rgb(0, 255, 0); }"
        );

    connect(appendModeButton, &QPushButton::clicked, this, [this](){
        if (m_isAppend)
            {
            m_isAppend = false;
        }
        else
            m_isAppend = true;
    });

    auto* demucsToggleButton = ui->DemucsToggleButton;
    demucsToggleButton->setStyleSheet(
        "QPushButton { background-color: rgb(0, 85, 0); }"
        "QPushButton::checked { background-color: rgb(0, 255, 0); }"
        );

    connect(demucsToggleButton, &QPushButton::clicked, this, [this]() {
        if (m_isDemucs)
            m_isDemucs = false;
        else
            m_isDemucs = true;
    });

    auto* nonSampledToggleButton = ui->NonSampledToggleButton;
    nonSampledToggleButton->setStyleSheet(
        "QPushButton { background-color: rgb(0, 85, 0); }"
        "QPushButton::checked { background-color: rgb(0, 255, 0); }"
        );

    connect(nonSampledToggleButton, &QPushButton::clicked, this, [this]() {
        if (m_isNonSampled)
            m_isNonSampled = false;
        else
            m_isNonSampled = true;
    });

    auto* interpolateModeButton = ui->InterpolateModeButton;
    interpolateModeButton->setStyleSheet(
        "QPushButton { background-color: rgb(0, 85, 0); }"
        "QPushButton::checked { background-color: rgb(0, 255, 0); }"
        );

    connect(interpolateModeButton, &QPushButton::clicked, this, [this](){
        if (m_isInterpolate) {
            m_isInterpolate = false;
        } else {
            m_isInterpolate = true;
        }
    });

    auto* chunkCrossfadeSpinBox = ui->ChunkCrossfadeSpinBox;

    connect(chunkCrossfadeSpinBox, &QSpinBox::valueChanged, this, [this, chunkCrossfadeSpinBox](){
        m_chunkCrossfadeSamples = chunkCrossfadeSpinBox->value();
    });

}

SamplerInfinite::~SamplerInfinite()
{
    delete ui;
}
