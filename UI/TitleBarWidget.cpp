#include "TitleBarWidget.h"
#include "ContourListPopup.h"

TitleBarWidget::TitleBarWidget(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    initUI();
    initConnections();
}

TitleBarWidget::~TitleBarWidget() {}

void TitleBarWidget::SetContourListProvider(
    std::function<std::vector<ROIDisplayInfo>()> provider)
{
    m_contourListProvider = std::move(provider);
}

// ============================================================
//  核心：一行绑定一个按钮和一个模式
// ============================================================

void TitleBarWidget::bindModeButton(QToolButton* button, InteractionMode mode) {
    connect(button, &QToolButton::toggled, this, [this, mode](bool state) {
        emit requestMode(mode, state);
        });
}

// ============================================================
//  信号连接
// ============================================================

void TitleBarWidget::initConnections()
{
    // 模式按钮：以后加新模式只需加一行
    bindModeButton(ui.NormalMode, InteractionMode::Normal);
    bindModeButton(ui.DistanceMeasurement, InteractionMode::DistanceMeasure);
    bindModeButton(ui.AngleMeasurement, InteractionMode::AngleMeasure);
    bindModeButton(ui.RoiMeasurement, InteractionMode::RegistrationROI);
    bindModeButton(ui.FreehandROI, InteractionMode::FreehandROI);
    bindModeButton(ui.CrosshairRuler, InteractionMode::CrosshairRuler);
    bindModeButton(ui.ImageDrag, InteractionMode::ImageDrag);

    // 重置视图（click，不是 toggle）
    connect(ui.ResetView, &QToolButton::clicked, this, &TitleBarWidget::requestResetViews);

    // 轮廓列表弹窗
    connect(ui.ContourOverlay, &QToolButton::toggled, this, [this](bool state) {
        if (state) {
            if (!m_contourPopup) {
                m_contourPopup = new ContourListPopup(this);
                connect(m_contourPopup, &ContourListPopup::roiVisibilityChanged,
                    this, &TitleBarWidget::roiVisibilityChanged);
            }
            if (m_contourListProvider) {
                m_contourPopup->SetROIList(m_contourListProvider());
            }
            QPoint pos = ui.ContourOverlay->mapToGlobal(
                QPoint(0, ui.ContourOverlay->height()));
            m_contourPopup->move(pos);
            m_contourPopup->show();
            m_contourPopup->raise();
            m_contourPopup->setFocus();
        }
        else {
            if (m_contourPopup) m_contourPopup->hide();
        }
        });
}

// ============================================================
//  UI 外观初始化
// ============================================================

void TitleBarWidget::initUI()
{
    QString buttonStyle = R"(
    QToolButton {
        background-color: #f0f0f0;
        border: 1px solid #d0d0d0;
        color: #333333;
        padding: 6px 12px;
        font-size: 10pt;
        border-radius: 4px;
        text-align: center;
    }

    QToolButton:hover {
        border: 1px solid #007ACC;
        background-color: #e6f0fa;
    }

    QToolButton:checked {
        background-color: #007ACC;
        color: white;
        border: 1px solid #005a9e;
    }

    QToolButton:pressed {
        background-color: #0062cc;
    }

    QToolButton:disabled {
        background-color: #f5f5f5;
        color: #aaaaaa;
        border: 1px solid #dddddd;
    }
)";

    this->setStyleSheet(buttonStyle);

    ui.OpenFolder->setStyleSheet(buttonStyle);
    ui.OpenFolder->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Folder.png"));

    ui.NormalMode->setStyleSheet(buttonStyle);
    ui.NormalMode->setIcon(QIcon(":/DicomImageStudy/images/reset.png"));

    ui.DistanceMeasurement->setStyleSheet(buttonStyle);
    ui.DistanceMeasurement->setIcon(QIcon(":/DicomImageStudy/images/Ruler.png"));

    ui.AngleMeasurement->setStyleSheet(buttonStyle);
    ui.AngleMeasurement->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Angle.png"));

    ui.ResetView->setStyleSheet(buttonStyle);
    ui.ResetView->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Calibration.png"));

    ui.RoiMeasurement->setStyleSheet(buttonStyle);
    ui.RoiMeasurement->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Frame.png"));

    ui.FreehandROI->setStyleSheet(buttonStyle);
    ui.FreehandROI->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Lasso.png"));

    ui.toolButton_7->setStyleSheet(buttonStyle);
    ui.toolButton_7->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Menu.png"));

    ui.ContourOverlay->setStyleSheet(buttonStyle);
    ui.ContourOverlay->setIcon(QIcon(":/DicomImageStudy/images/FlatStyle-Squares.png"));

    ui.ImageDrag->setStyleSheet(buttonStyle);
    ui.ImageDrag->setIcon(QIcon(":/DicomImageStudy/images/move.png"));

    ui.CrosshairRuler->setStyleSheet(buttonStyle);
    ui.CrosshairRuler->setIcon(QIcon(":/DicomImageStudy/images/crosshair1.png"));
}