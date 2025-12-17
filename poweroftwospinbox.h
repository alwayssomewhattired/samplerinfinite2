#ifndef POWEROFTWOSPINBOX_H
#define POWEROFTWOSPINBOX_H

#include <QSpinBox>

class PowerOfTwoSpinBox : public QSpinBox
{
    Q_OBJECT
public:

    explicit PowerOfTwoSpinBox(QWidget *parent = nullptr);


protected:

    void stepBy(int steps) override;

    int valueFromText(const QString& text) const override;

};

#endif // POWEROFTWOSPINBOX_H
