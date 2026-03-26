#include "LeftToolWidget.h"

#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QObject>

// ============================================================
//  构造 / 析构
// ============================================================

LeftToolWidget::LeftToolWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LeftToolWidgetClass)
{
    ui->setupUi(this);
    InitDicomInfoTab();
}

LeftToolWidget::~LeftToolWidget()
{
    delete ui;
}

// ============================================================
//  Tab 1 初始化
//  QTableWidget 动态性强（行数不定），在代码中创建比在 .ui 中更灵活。
//  Tab 2 的所有控件均已在 .ui 中设计，无需代码创建。
// ============================================================

void LeftToolWidget::InitDicomInfoTab()
{
    m_dicomTable = new QTableWidget(ui->tab);
    m_dicomTable->setColumnCount(2);
    m_dicomTable->setHorizontalHeaderLabels({
        tr("key"),
        tr("value")
        });
    m_dicomTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dicomTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_dicomTable->setAlternatingRowColors(true);
    m_dicomTable->verticalHeader()->setVisible(false);
    m_dicomTable->horizontalHeader()->setStretchLastSection(true);
    m_dicomTable->setColumnWidth(0, 120);
    m_dicomTable->setObjectName("dicomInfoTable");

    // 直接加入 .ui 中已有的布局
    if (ui->tab->layout()) {
        ui->tab->layout()->addWidget(m_dicomTable);
    }
    else {
        auto* lay = new QVBoxLayout(ui->tab);
        lay->setContentsMargins(5, 5, 5, 5);
        lay->addWidget(m_dicomTable);
    }
}

// ============================================================
//  Tab 1 数据更新
// ============================================================

void LeftToolWidget::SetDicomMetadata(const QMap<QString, QString>& metadata)
{
    if (!m_dicomTable) return;

    m_dicomTable->clearContents();
    m_dicomTable->setRowCount(metadata.size());
    m_dicomTable->setHorizontalHeaderLabels({
        tr("key"),
        tr("value")
        });

    int row = 0;
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
        m_dicomTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_dicomTable->setItem(row, 1, new QTableWidgetItem(it.value()));
        ++row;
    }
    m_dicomTable->resizeRowsToContents();
}

// ============================================================
//  Tab 2 数据更新
// ============================================================

void LeftToolWidget::OnDragUpdated(int    viewIndex,
    double dx, double dy, double dz,
    double totalDist)
{
    // ---- 更新视图名称 ----
    static const QString  kViewNames[] = {
        QStringLiteral("轴状位（Axial）"),
        QStringLiteral("矢状位（Sagittal）"),
        QStringLiteral("冠状位（Coronal）")
    };
    if (viewIndex >= 0 && viewIndex < 3) {
        ui->labelViewName->setText(tr(kViewNames[viewIndex].toStdString().c_str()));
    }

    // ---- 格式化位移数值（保留两位小数，显示正负号）----
    auto formatValue = [](double v) -> QString {
        return QString("%1%2 mm")
            .arg(v >= 0.0 ? "+" : "")
            .arg(v, 0, 'f', 2);
        };

    ui->labelDx->setText(formatValue(dx));
    ui->labelDy->setText(formatValue(dy));
    ui->labelDz->setText(formatValue(dz));
    ui->labelTotal->setText(QString("%1 mm").arg(totalDist, 0, 'f', 2));

    // ---- 根据方向动态着色（正=蓝 负=橙 零=默认）----
    auto applyColor = [](QLabel* label, double v) {
        const QString base =
            "font-family: Consolas, monospace; font-size: 11pt; font-weight: bold;";
        if (qAbs(v) < 0.01) {
            label->setStyleSheet(base);
        }
        else if (v > 0.0) {
            label->setStyleSheet(base + " color: #007ACC;");   // 蓝色
        }
        else {
            label->setStyleSheet(base + " color: #E07020;");   // 橙色
        }
        };

    applyColor(ui->labelDx, dx);
    applyColor(ui->labelDy, dy);
    applyColor(ui->labelDz, dz);

    // ---- 自动切换到拖动信息 Tab ----
    ui->tabWidget->setCurrentWidget(ui->tab_2);
}

void LeftToolWidget::OnDragReset()
{
    ui->labelViewName->setText(tr("—"));

    const QString zero = tr("0.00 mm");
    ui->labelDx->setText(zero);
    ui->labelDy->setText(zero);
    ui->labelDz->setText(zero);
    ui->labelTotal->setText(zero);

    // 恢复默认颜色
    const QString baseStyle =
        "font-family: Consolas, monospace; font-size: 11pt; font-weight: bold;";
    ui->labelDx->setStyleSheet(baseStyle);
    ui->labelDy->setStyleSheet(baseStyle);
    ui->labelDz->setStyleSheet(baseStyle);
    ui->labelTotal->setStyleSheet(
        "font-family: Consolas, monospace; font-size: 13pt; "
        "font-weight: bold; color: #007ACC;");
}
