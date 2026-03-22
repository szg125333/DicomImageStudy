#include "ImageDragStrategy.h"
#include "Interface/IViewController.h"
#include "Interface/IViewRenderer.h"

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkImageViewer2.h>
#include <vtkRenderWindow.h>
#include <vtkImageViewer2.h>

#include <cmath>
#include <algorithm>

ImageDragStrategy::ImageDragStrategy(IViewController* controller)
    : IInteractionStrategy(controller)
{
}

// ============================================================
//  事件分发
// ============================================================

void ImageDragStrategy::HandleEvent(EventType        type,
    int              viewIndex,
    const EventData& data)
{
    if (!m_controller) return;

    // 视图锁定
    if (m_activeViewIndex != -1 && m_activeViewIndex != viewIndex) return;

    auto* renderer = m_controller->GetRenderer(viewIndex);
    if (!renderer) return;

    switch (type) {

        // ── 左键按下：记录起始屏幕像素坐标 ─────────────────────────────
    case EventType::LeftPress: {
        m_isDragging = true;
        m_activeViewIndex = viewIndex;
        // 只记录屏幕坐标，不做 Picker 拾取
        m_lastScreenX = data.mousePosX;
        m_lastScreenY = data.mousePosY;
        break;
    }

                             // ── 左键拖动：屏幕像素增量 → 世界坐标增量 → 平移相机 ─────────
    case EventType::LeftMove: {
        if (!m_isDragging) break;

        const int pixDx = data.mousePosX - m_lastScreenX;
        const int pixDy = data.mousePosY - m_lastScreenY;

        // 无位移则跳过
        if (pixDx == 0 && pixDy == 0) break;

        // 更新屏幕基准（立即更新，不依赖渲染）
        m_lastScreenX = data.mousePosX;
        m_lastScreenY = data.mousePosY;

        // ── 获取相机和视口尺寸 ──────────────────────────────────────
        auto viewer = renderer->GetViewer();
        if (!viewer || !viewer->GetRenderer()) break;

        auto* camera = viewer->GetRenderer()->GetActiveCamera();
        if (!camera || !camera->GetParallelProjection()) break;

        auto* window = viewer->GetRenderWindow();
        if (!window) break;

        const int* winSize = window->GetSize();
        if (winSize[0] <= 0 || winSize[1] <= 0) break;

        // ── 核心换算：像素 → 世界坐标（正交投影公式）────────────────
        //
        // ParallelScale = 视口高度一半对应的世界坐标长度（mm）
        // 整个视口高度 = 2 × ParallelScale（mm）
        // 单像素对应 = 2 × ParallelScale / viewportHeightPixels
        //
        const double parallelScale = camera->GetParallelScale();
        const double worldPerPixelY = (2.0 * parallelScale) / static_cast<double>(winSize[1]);
        // X 方向考虑宽高比
        const double aspect = static_cast<double>(winSize[0]) / static_cast<double>(winSize[1]);
        const double worldPerPixelX = worldPerPixelY * aspect;

        // 屏幕像素增量 → 世界坐标增量
        // VTK 屏幕 Y 轴向上为正，鼠标向下 pixDy 为负，需要取反
        const double worldDeltaX = pixDx * worldPerPixelX;
        const double worldDeltaY = -pixDy * worldPerPixelY;   // 取反：屏幕向下 = 世界向下

        // ── 确定相机在世界坐标中的"右方向"和"上方向" ─────────────
        // 正交投影下 vtkImageViewer2 的视图向量由切片方向决定，
        // 直接用相机的 ViewUp 和右向量（ViewUp × ViewDir 的叉积）来映射。
        double viewUp[3], viewDir[3], rightVec[3];
        camera->GetViewUp(viewUp);
        camera->GetDirectionOfProjection(viewDir);   // 相机看向的方向（单位向量）

        // 右向量 = ViewDir × ViewUp（右手定则）
        rightVec[0] = viewDir[1] * viewUp[2] - viewDir[2] * viewUp[1];
        rightVec[1] = viewDir[2] * viewUp[0] - viewDir[0] * viewUp[2];
        rightVec[2] = viewDir[0] * viewUp[1] - viewDir[1] * viewUp[0];

        // 归一化
        double lenR = std::sqrt(rightVec[0] * rightVec[0] +
            rightVec[1] * rightVec[1] +
            rightVec[2] * rightVec[2]);
        if (lenR < 1e-6) break;
        rightVec[0] /= lenR; rightVec[1] /= lenR; rightVec[2] /= lenR;

        // 最终世界空间位移向量：向右移 worldDeltaX，向上移 worldDeltaY
        const double wdx = rightVec[0] * worldDeltaX + viewUp[0] * worldDeltaY;
        const double wdy = rightVec[1] * worldDeltaX + viewUp[1] * worldDeltaY;
        const double wdz = rightVec[2] * worldDeltaX + viewUp[2] * worldDeltaY;

        // ── 平移相机（焦点 + 位置同步，图像跟随）────────────────────
        // 相机反向移动 → 图像向拖动方向移动
        double focal[3], pos[3];
        camera->GetFocalPoint(focal);
        camera->GetPosition(pos);

        camera->SetFocalPoint(focal[0] - wdx, focal[1] - wdy, focal[2] - wdz);
        camera->SetPosition(pos[0] - wdx, pos[1] - wdy, pos[2] - wdz);
        viewer->GetRenderer()->ResetCameraClippingRange();

        // ── 累计物理位移（mm）───────────────────────────────────────
        // 注意：累计的是图像的移动方向（与相机移动方向相反）
        m_totalDx += wdx;
        m_totalDy += wdy;
        m_totalDz += wdz;

        const double total = std::sqrt(
            m_totalDx * m_totalDx +
            m_totalDy * m_totalDy +
            m_totalDz * m_totalDz);

        if (m_dragCb) {
            m_dragCb(viewIndex, m_totalDx, m_totalDy, m_totalDz, total);
        }

        renderer->RequestRender();
        break;
    }

                            // ── 左键释放：结束拖动 ────────────────────────────────────────
    case EventType::LeftRelease: {
        m_isDragging = false;
        m_activeViewIndex = -1;
        break;
    }

                               // ── 右键单击：清零累计位移 ────────────────────────────────────
    case EventType::RightPress: {
        ResetDisplacement();
        break;
    }

    default:
        break;
    }
}

// ============================================================
//  清除
// ============================================================

void ImageDragStrategy::Clear(int /*viewIndex*/)
{
    m_isDragging = false;
    m_activeViewIndex = -1;
    ResetDisplacement();
}

// ============================================================
//  私有
// ============================================================

void ImageDragStrategy::ResetDisplacement()
{
    m_totalDx = 0.0;
    m_totalDy = 0.0;
    m_totalDz = 0.0;
    if (m_resetCb) m_resetCb();
}
