#pragma once
#include "IInteractionStrategy.h"
#include <array>
#include "Common/Measurement/MeasurementTypes.h"

/// @brief 角度测量工具交互策略
/// 
/// 处理角度测量模式下的交互事件

enum class EditStatus {
    startPonit = 0,
    middlePoint = 1,
    endPonit = 2,
	None = 3
};

class AngleMeasureStrategy : public IInteractionStrategy {
public:
    explicit AngleMeasureStrategy(IViewController* controller);
    ~AngleMeasureStrategy() override = default;

    void HandleEvent(EventType type, int viewIndex, const EventData& data) override;
    void Clear(int viewIndex) override;

private:
	EditStatus m_editStatus = EditStatus::None;
    std::array<double, 3> m_startWorldPos;
    std::array<double, 3> m_middleWorldPos;

	EditableAnglePoint m_currentEditablePoint; // 当前拾取到的可编辑点信息

    int m_startViewIndex = -1;

    bool m_isEditing = false;
    int m_editingMeasurementId = -1;
    int m_editingIsStart = 0;
    int m_editingViewIndex = -1;
};