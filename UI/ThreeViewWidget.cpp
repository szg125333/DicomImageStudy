#include "ThreeViewWidget.h"

#include <QGridLayout>
#include <QVTKOpenGLNativeWidget.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <QSplitter>

#include "Renderer/VtkViewRenderer.h"        // 你的 VTK 后端实现
#include "Controller/ThreeViewController.h"    // 控制器实现
#include "Interface/IViewRenderer.h"

#include <vtkImageData.h>

ThreeViewWidget::ThreeViewWidget(QWidget* parent, ThreeViewController* controller)
    : QWidget(parent), m_controller(controller)
{
    setupUi();
    setupRenderersAndController();
}

ThreeViewWidget::~ThreeViewWidget()
{
    // 如果 widget 创建并拥有 controller，则负责删除
    if (m_controllerOwned && m_controller) {
        delete m_controller;
        m_controller = nullptr;
    }
    // m_renderers 使用 unique_ptr 自动释放
}

void ThreeViewWidget::setupUi()
{
    // 根布局仍然设置在 this 上
    auto rootLayout = new QHBoxLayout(this);
    rootLayout->setSpacing(2);
    rootLayout->setContentsMargins(2, 2, 2, 2);

    // 左侧单个 widget
    m_widgets[0] = new QVTKOpenGLNativeWidget(this);

    // 右侧上下两个 widget
    m_widgets[1] = new QVTKOpenGLNativeWidget(this);
    m_widgets[2] = new QVTKOpenGLNativeWidget(this);

    // 设置大小策略，确保渲染控件能扩展
    for (int i = 0; i < 3; ++i) {
        if (m_widgets[i]) {
            m_widgets[i]->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }
    }

    // 右侧使用垂直 splitter 或垂直布局包裹
    auto rightSplitter = new QSplitter(Qt::Vertical, this);
    rightSplitter->addWidget(m_widgets[1]);
    rightSplitter->addWidget(m_widgets[2]);
    rightSplitter->setChildrenCollapsible(false);

    // 左右使用水平 splitter，使得用户可以拖动调整左右宽度
    auto mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->addWidget(m_widgets[0]);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setChildrenCollapsible(false);

    // 初始大小比例：左侧占一半，右侧占一半（右侧内部上下各半）
    // setSizes 接受像素值数组，比例可通过窗口初始宽度计算或直接设置近似值
    mainSplitter->setSizes({ 600, 600 }); // 假设初始总宽约1200
    rightSplitter->setSizes({ 300, 300 }); // 假设右侧高度约600

    // 将 splitter 放入根布局
    rootLayout->addWidget(mainSplitter);
}

void ThreeViewWidget::setupRenderersAndController()
{
    // 创建 renderer 并绑定到对应的 QVTK 控件
    for (int i = 0; i < 3; ++i) {
        m_renderers[i] = std::make_unique<VtkViewRenderer>(m_widgets[i]);
    }

    // 设置方向
    m_renderers[static_cast<int>(ViewType::Axial)]->SetOrientation(SliceOrientation::XY);
    m_renderers[static_cast<int>(ViewType::Sagittal)]->SetOrientation(SliceOrientation::YZ);
    m_renderers[static_cast<int>(ViewType::Coronal)]->SetOrientation(SliceOrientation::XZ);

    // 如果外部没有注入 controller，则由 widget 创建并拥有它
    if (!m_controller) {
        m_controller = new ThreeViewController(this);
        m_controllerOwned = true;
    }

    // 将 renderer 指针数组传给 controller
    std::array<IViewRenderer*, 3> arr = {
        m_renderers[static_cast<int>(ViewType::Axial)].get(),
        m_renderers[static_cast<int>(ViewType::Sagittal)].get(),
        m_renderers[static_cast<int>(ViewType::Coronal)].get()
    };

    m_controller->SetRenderers(arr);

    // 转发 controller 的 sliceChanged 信号到 widget 的信号
    connect(m_controller, &ThreeViewController::sliceChanged,
        this, [this](int viewIndex, int slice) {
            emit sliceChanged(viewIndex, slice);
        });
}

void ThreeViewWidget::SetImageData(vtkImageData* image)
{
    if (!m_controller) return;
    m_controller->SetImageData(image);
}

void ThreeViewWidget::RequestSetSlice(ViewType view, int slice)
{
    if (!m_controller) return;
    m_controller->RequestSetSlice(view, slice);
}

void ThreeViewWidget::setModeToDistanceMeasurement(bool state)
{
    if (!state) {
       
    }
	m_controller->SetInteractionMode(InteractionMode::DistanceMeasure);
}
