

#ifndef SearchableComboBox_H
#define SearchableComboBox_H
// replace with pragma once
#include <QWidget>
#include <QLineEdit>
#include <QListWidget>

class SearchableComboBox : public QWidget
{
    Q_OBJECT
public:
    explicit SearchableComboBox(QWidget *parent = nullptr);

    QString selected() const;

    void freqsSetter();

signals:
    void itemSelected(const QString &item);

private:
    QLineEdit *searchBox;
    QListWidget *listWidget;
    QString selectedItem;
};

#endif // SearchableComboBox_H
