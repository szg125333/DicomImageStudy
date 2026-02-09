基于 Qt + VTK 的医学图像（DICOM/NIfTI 等）三视图（轴位/矢状/冠状）同步可视化系统，支持窗宽窗位调整、十字线定位、切片联动，并采用接口抽象与策略模式，便于扩展交互功能（如测量、标注等）。

🧠 1. 项目简介

- ✅ 三视图同步显示：Axial / Sagittal / Coronal 切片联动
- ✅ 十字线定位：点击任一视图，自动在其他两视图标出对应位置
- ✅ 窗宽窗位（Window/Level）实时调整
- ✅ 完全解耦架构：UI、Controller、Renderer 通过抽象接口通信
- ✅ 策略模式支持：可轻松切换交互模式（如普通浏览、距离测量等）
- ✅ Overlay 可插拔：十字线、文本信息等渲染元素模块化管理
- ✅ 跨平台：支持 Windows / Linux / macOS（依赖 Qt + VTK）

🏗️ 2. 架构设计

- <img width="701" height="598" alt="image" src="https://github.com/user-attachments/assets/44dcb475-5d32-4d4c-9e75-699cb872964b" />

🔗 3. 模块依赖关系图（Mermaid）
