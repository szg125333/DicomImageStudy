#include "LeftToolWidget.h"

#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFrame>
#include <QString>

// ============================================================
//  构造 / 析构
// ============================================================

LeftToolWidget::LeftToolWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::LeftToolWidgetClass)
{
    ui->setupUi(this);
    InitDicomInfoTab();
    InitDragInfoTab();
}

LeftToolWidget::~LeftToolWidget()
{
    delete ui;
}

// ============================================================
//  Tab 1 初始化 —— DICOM 元数据
// ============================================================

void LeftToolWidget::InitDicomInfoTab()
{
    m_dicomTable = new QTableWidget(ui->tab);
    m_dicomTable->setColumnCount(2);
    m_dicomTable->setHorizontalHeaderLabels({ "属性", "值" });
    m_dicomTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_dicomTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_dicomTable->setAlternatingRowColors(true);
    m_dicomTable->verticalHeader()->setVisible(false);
    m_dicomTable->horizontalHeader()->setStretchLastSection(true);
    m_dicomTable->setColumnWidth(0, 120);
    m_dicomTable->setObjectName("dicomInfoTable");

    if (ui->tab->layout()) {
        ui->tab->layout()->addWidget(m_dicomTable);
    }
    else {
        auto* lay = new QVBoxLayout(ui->tab);
        lay->setContentsMargins(5, 5, 5, 5);
        lay->addWidget(m_dicomTable);
    }

    // 修改 Tab 标题为中文
    ui->tabWidget->setTabText(0, "DICOM 信息");
}

// ============================================================
//  Tab 2 初始化 —— 拖动位移面板
// ============================================================

void LeftToolWidget::InitDragInfoTab()
{
    // ---- 将 tab_2 的标题改为"拖动信息" ----
    ui->tabWidget->setTabText(1, "拖动信息");

    // ---- 外层布局 ----
    auto* outerLayout = new QVBoxLayout(ui->tab_2);
    outerLayout->setContentsMargins(8, 8, 8, 8);
    outerLayout->setSpacing(10);

    // ====================================================
    //  分组框：当前视图
    // ====================================================
    auto* groupView = new QGroupBox("当前操作视图", ui->tab_2);
    auto* gridView = new QGridLayout(groupView);
    gridView->setContentsMargins(8, 12, 8, 8);
    gridView->setSpacing(6);

    auto* lblViewTitle = new QLabel("视图：", groupView);
    m_labelView = new QLabel("—", groupView);
    m_labelView->setStyleSheet("font-weight: bold; color: #007ACC;");
    gridView->addWidget(lblViewTitle, 0, 0);
    gridView->addWidget(m_labelView, 0, 1);

    outerLayout->addWidget(groupView);

    // ====================================================
    //  分组框：各轴位移
    // ====================================================
    auto* groupDist = new QGroupBox("累计位移（mm）", ui->tab_2);
    auto* gridDist = new QGridLayout(groupDist);
    gridDist->setContentsMargins(8, 12, 8, 8);
    gridDist->setSpacing(8);

    // 表头
    auto* hAxis = new QLabel("轴", groupDist); hAxis->setStyleSheet("color: gray; font-size: 9pt;");
    auto* hValue = new QLabel("位移", groupDist); hValue->setStyleSheet("color: gray; font-size: 9pt;");
    gridDist->addWidget(hAxis, 0, 0);
    gridDist->addWidget(hValue, 0, 1);

    // 分隔线
    auto* line = new QFrame(groupDist);
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    gridDist->addWidget(line, 1, 0, 1, 2);

    // X 轴
    auto* lblX = new QLabel("X 轴（左右）：", groupDist);
    m_labelDx = new QLabel("0.00 mm", groupDist);
    m_labelDx->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_labelDx->setStyleSheet("font-family: monospace; font-size: 11pt; font-weight: bold;");
    gridDist->addWidget(lblX, 2, 0);
    gridDist->addWidget(m_labelDx, 2, 1);

    // Y 轴
    auto* lblY = new QLabel("Y 轴（前后）：", groupDist);
    m_labelDy = new QLabel("0.00 mm", groupDist);
    m_labelDy->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_labelDy->setStyleSheet("font-family: monospace; font-size: 11pt; font-weight: bold;");
    gridDist->addWidget(lblY, 3, 0);
    gridDist->addWidget(m_labelDy, 3, 1);

    // Z 轴
    auto* lblZ = new QLabel("Z 轴（上下）：", groupDist);
    m_labelDz = new QLabel("0.00 mm", groupDist);
    m_labelDz->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_labelDz->setStyleSheet("font-family: monospace; font-size: 11pt; font-weight: bold;");
    gridDist->addWidget(lblZ, 4, 0);
    gridDist->addWidget(m_labelDz, 4, 1);

    // 分隔线
    auto* line2 = new QFrame(groupDist);
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    gridDist->addWidget(line2, 5, 0, 1, 2);

    // 总位移
    auto* lblTotal = new QLabel("总位移（3D）：", groupDist);
    lblTotal->setStyleSheet("font-weight: bold;");
    m_labelTotal = new QLabel("0.00 mm", groupDist);
    m_labelTotal->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_labelTotal->setStyleSheet(
        "font-family: monospace; font-size: 13pt; font-weight: bold; color: #007ACC;");
    gridDist->addWidget(lblTotal, 6, 0);
    gridDist->addWidget(m_labelTotal, 6, 1);

    outerLayout->addWidget(groupDist);

    // ====================================================
    //  操作提示
    // ====================================================
    m_labelHint = new QLabel(
        "<span style='color:gray; font-size:9pt;'>"
        "💡 左键拖动平移图像<br>"
        "   右键单击清零位移"
        "</span>",
        ui->tab_2);
    m_labelHint->setWordWrap(true);
    m_labelHint->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    outerLayout->addWidget(m_labelHint);

    // 底部弹性填充，防止控件被拉伸
    outerLayout->addStretch();
}

// ============================================================
//  Tab 1 —— 数据更新
// ============================================================

void LeftToolWidget::SetDicomMetadata(const QMap<QString, QString>& metadata)
{
    if (!m_dicomTable) return;

    m_dicomTable->clear();
    m_dicomTable->setRowCount(metadata.size());
    m_dicomTable->setHorizontalHeaderLabels({ "属性", "值" });

    int row = 0;
    for (auto it = metadata.constBegin(); it != metadata.constEnd(); ++it) {
        m_dicomTable->setItem(row, 0, new QTableWidgetItem(it.key()));
        m_dicomTable->setItem(row, 1, new QTableWidgetItem(it.value()));
        ++row;
    }
    m_dicomTable->resizeRowsToContents();
}

// ============================================================
//  Tab 2 —— 拖动信息更新
// ============================================================

void LeftToolWidget::OnDragUpdated(int    viewIndex,
    double dx, double dy, double dz,
    double totalDist)
{
    // 更新视图名称
    const char* viewNames[] = { "轴状位（Axial）", "矢状位（Sagittal）", "冠状位（Coronal）" };
    if (viewIndex >= 0 && viewIndex < 3) {
        m_labelView->setText(viewNames[viewIndex]);
    }

    // 格式：保留两位小数，正数显示 "+"，负数显示 "-"
    auto fmt = [](double v) -> QString {
        return QString("%1%2 mm")
            .arg(v >= 0 ? "+" : "")
            .arg(v, 0, 'f', 2);
        };

    m_labelDx->setText(fmt(dx));
    m_labelDy->setText(fmt(dy));
    m_labelDz->setText(fmt(dz));
    m_labelTotal->setText(QString("%1 mm").arg(totalDist, 0, 'f', 2));

    // 根据位移方向动态着色（正 = 蓝，负 = 橙，零 = 灰）
    auto colorStyle = [](double v) -> QString {
        if (std::abs(v) < 0.01) return "font-family:monospace;font-size:11pt;font-weight:bold;color:gray;";
        return v > 0
            ? "font-family:monospace;font-size:11pt;font-weight:bold;color:#007ACC;"
            : "font-family:monospace;font-size:11pt;font-weight:bold;color:#E07020;";
        };

    m_labelDx->setStyleSheet(colorStyle(dx));
    m_labelDy->setStyleSheet(colorStyle(dy));
    m_labelDz->setStyleSheet(colorStyle(dz));

    // 自动切换到拖动信息 Tab
    ui->tabWidget->setCurrentWidget(ui->tab_2);
}

void LeftToolWidget::OnDragReset()
{
    m_labelView->setText("—");
    m_labelDx->setText("0.00 mm");
    m_labelDy->setText("0.00 mm");
    m_labelDz->setText("0.00 mm");
    m_labelTotal->setText("0.00 mm");

    const QString baseStyle =
        "font-family: monospace; font-size: 11pt; font-weight: bold;";
    m_labelDx->setStyleSheet(baseStyle);
    m_labelDy->setStyleSheet(baseStyle);
    m_labelDz->setStyleSheet(baseStyle);
    m_labelTotal->setStyleSheet(
        "font-family: monospace; font-size: 13pt; font-weight: bold; color: #007ACC;");
}
