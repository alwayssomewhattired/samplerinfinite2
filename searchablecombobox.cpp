#include "searchablecombobox.h"
#include <QVBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include "frequencies.h"

SearchableComboBox::SearchableComboBox(QWidget *parent) : QWidget(parent)
{
    Frequencies freq;
    auto *layout = new QVBoxLayout(this);
    searchBox = new QLineEdit(this);
    listWidget = new QListWidget(this);
    listWidget->addItems({"all"});
    listWidget->addItems(freq.getterNotes());

    layout->addWidget(searchBox);
    layout->addWidget(listWidget);
    setLayout(layout);

    connect(searchBox, &QLineEdit::textChanged, this, [this](const QString &text){
        for (int i = 0; i < listWidget->count(); ++i)
        {
            QListWidgetItem *item = listWidget->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });

    connect(listWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem *item){
        selectedItem = item->text();
        emit itemSelected(selectedItem);
    });
}

QString SearchableComboBox::selected() const { return selectedItem; }
