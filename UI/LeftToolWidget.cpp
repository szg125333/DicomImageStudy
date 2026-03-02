#include "LeftToolWidget.h"
#include "ui_LeftToolWidget.h"

#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QDebug>

LeftToolWidget::LeftToolWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LeftToolWidgetClass)
{
    ui->setupUi(this);

    // === 在 tabInfo 中创建 QTableWidget ===
    QTableWidget* table = new QTableWidget(ui->tab);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({ "Property", "Value" });
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnWidth(0, 120); // 固定第一列宽度

    // ⚠️ 关键：直接添加到已存在的布局（Designer 中已设置）
    if (ui->tab->layout()) {
        ui->tab->layout()->addWidget(table);
    }
    else {
        // 如果 Designer 忘记设布局，这里兜底（但不推荐）
        QVBoxLayout* layout = new QVBoxLayout(ui->tab);
        layout->addWidget(table);
        layout->setContentsMargins(5, 5, 5, 5);
    }

    // 保存指针（可通过 ui->tabInfo->findChild<QTableWidget*>() 获取，但直接存更好）
    table->setObjectName("dicomInfoTable");
}

LeftToolWidget::~LeftToolWidget()
{
    delete ui;
}

void LeftToolWidget::SetDicomMetadata(const QMap<QString, QString>& metadata)
{
    QTableWidget* table = ui->tab->findChild<QTableWidget*>("dicomInfoTable");
    if (!table) return;

    table->clear();
    table->setRowCount(metadata.size());
    table->setHorizontalHeaderLabels({ "Property", "Value" });

    int row = 0;
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
        table->setItem(row, 0, new QTableWidgetItem(it.key()));
        table->setItem(row, 1, new QTableWidgetItem(it.value()));
        ++row;
    }

    table->resizeRowsToContents();
}