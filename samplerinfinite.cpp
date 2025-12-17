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

        filePaths.push_back(filePath.toStdString());

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
        m_backend.process(freqDisplayWidget->toPlainText(), filePaths, m_frequencies.getterNoteToFreq(), m_frequencies.getFreqToNote(),
        m_frequencies.get_i_freqToNote(), m_isAppend, m_isInterpolate, m_isDemucs, m_isNonSampled, m_chunkCrossfadeSamples);

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
            m_backend.setOutputDirectory(dir.toStdString());
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
