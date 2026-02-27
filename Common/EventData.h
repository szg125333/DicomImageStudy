#pragma once

#include <string>

struct EventData {
    // 鼠标相关（所有鼠标事件都有）
    int mousePosX = 0;
    int mousePosY = 0;

    // 键盘相关（键盘事件有效）
    std::string keySym;  // 来自 VTK KeySym，如 "a", "Return", "Delete"

    // 修饰键状态（所有事件都记录！）
    bool ctrlPressed = false;
    bool shiftPressed = false;
    bool altPressed = false;

};