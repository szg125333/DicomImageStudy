#include "ContourListPopup.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFocusEvent>
#include <QFrame>

ContourListPopup::ContourListPopup(QWidget* parent)
    : QWidget(parent, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    // 设置为 Popup 窗口：点击外部自动关闭
    setAttribute(Qt::WA_DeleteOnClose, false);  // 不要点关闭就删除，可以复用
    setFocusPolicy(Qt::StrongFocus);

    buildUI();
}

void ContourListPopup::buildUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(4);

    // 标题栏
    auto* titleLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("Contour list");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 11pt; color: #333;");
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    // 全选/全不选按钮
    auto* showAllBtn = new QPushButton("All");
    showAllBtn->setFixedSize(40, 22);
    showAllBtn->setStyleSheet("font-size: 9pt; padding: 2px;");
    connect(showAllBtn, &QPushButton::clicked, [this]() {
        for (auto& kv : m_checkboxes) {
            kv.second->setChecked(true);
        }
        emit allVisibilityChanged(true);
        });

    auto* hideAllBtn = new QPushButton("None");
    hideAllBtn->setFixedSize(40, 22);
    hideAllBtn->setStyleSheet("font-size: 9pt; padding: 2px;");
    connect(hideAllBtn, &QPushButton::clicked, [this]() {
        for (auto& kv : m_checkboxes) {
            kv.second->setChecked(false);
        }
        emit allVisibilityChanged(false);
        });

    titleLayout->addWidget(showAllBtn);
    titleLayout->addWidget(hideAllBtn);
    m_mainLayout->addLayout(titleLayout);

    // 分割线
    auto* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("color: #d0d0d0;");
    m_mainLayout->addWidget(separator);

    // 滚动区域
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setMaximumHeight(300);

    m_listContainer = new QWidget();
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(2);

    m_scrollArea->setWidget(m_listContainer);
    m_mainLayout->addWidget(m_scrollArea);

    // 样式
    setStyleSheet(R"(
        ContourListPopup {
            background-color: #ffffff;
            border: 1px solid #c0c0c0;
            border-radius: 6px;
        }
    )");

    setMinimumWidth(220);
}

void ContourListPopup::SetROIList(const std::vector<ROIDisplayInfo>& roiList) {
    Clear();

    for (const auto& roi : roiList) {
        auto* item = createROIItem(roi);
        m_listLayout->addWidget(item);
    }

    m_listLayout->addStretch();
    adjustSize();
}

void ContourListPopup::Clear() {
    m_checkboxes.clear();

    // 清空 listLayout 中的所有 widget
    while (m_listLayout->count() > 0) {
        auto* item = m_listLayout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
}

QWidget* ContourListPopup::createROIItem(const ROIDisplayInfo& roi) {
    auto* itemWidget = new QWidget();
    auto* layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);

    // 色块
    auto* colorBlock = new QLabel();
    colorBlock->setFixedSize(14, 14);
    int r = static_cast<int>(roi.color[0] * 255);
    int g = static_cast<int>(roi.color[1] * 255);
    int b = static_cast<int>(roi.color[2] * 255);
    colorBlock->setStyleSheet(
        QString("background-color: rgb(%1, %2, %3); border: 1px solid #999; border-radius: 2px;")
        .arg(r).arg(g).arg(b));
    layout->addWidget(colorBlock);

    // Checkbox + 名称
    auto* checkbox = new QCheckBox(QString::fromStdString(roi.name));
    checkbox->setChecked(roi.visible);
    checkbox->setStyleSheet("font-size: 10pt; color: #333;");

    int roiNumber = roi.roiNumber;
    connect(checkbox, &QCheckBox::toggled, [this, roiNumber](bool checked) {
        emit roiVisibilityChanged(roiNumber, checked);
        });

    layout->addWidget(checkbox);
    layout->addStretch();

    m_checkboxes[roiNumber] = checkbox;

    return itemWidget;
}

// 删掉 focusOutEvent，改用 changeEvent 检测窗口失活
void ContourListPopup::changeEvent(QEvent* event) {
    if (event->type() == QEvent::ActivationChange) {
        if (!isActiveWindow()) {
            hide();
        }
    }
    QWidget::changeEvent(event);
}