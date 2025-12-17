#include "poweroftwospinbox.h"
#include <QStyleOptionSpinBox>
#include <QPainter>

PowerOfTwoSpinBox::PowerOfTwoSpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    setRange(1, 16384);
}

void PowerOfTwoSpinBox::stepBy(int steps) {
    int v = value();

    if (steps > 0) {
        v <<= 1;
    } else {
        v >>= 1;
    }

    if (v < minimum()) v = minimum();
    if (v > maximum()) v = maximum();

    setValue(v);
}

int PowerOfTwoSpinBox::valueFromText(const QString& text) const {
    bool ok = false;
    int v = text.toInt(&ok);

    if (!ok || v < 1) return 1;

    int p = 1;
    while (p < v) p <<= 1;

    int prev = p >> 1;
    if (prev < 1) prev = 1;

    return (std::abs(prev - v) < std::abs(p - v)) ? prev : p;
}

