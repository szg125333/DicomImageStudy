#pragma once

// 在类外或命名空间内
enum class AnglePointRole {
	None = 0,
	Start,
	Middle, // 顶点
	End
};

struct EditableAnglePoint {
	int measurementId = -1;
	AnglePointRole role = AnglePointRole::None;
};