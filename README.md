# DicomImageStudy 项目介绍

## 1. 项目概述

DicomImageStudy 是一个基于 Qt + VTK + ITK + GDCM 的医学影像查看与分析平台。系统采用三视图（Axial / Sagittal / Coronal）联动显示 DICOM CT 图像，并支持 RT Structure 轮廓叠加、测量工具、图像配准等放射治疗相关功能。

### 技术栈

| 组件 | 技术 | 用途 |
|------|------|------|
| UI 框架 | Qt 5/6 + QVTKOpenGLNativeWidget | 窗口、工具栏、VTK 渲染嵌入 |
| 图像渲染 | VTK (vtkImageViewer2) | 三视图切片显示、Overlay 叠加 |
| 图像读取 | ITK + GDCM | DICOM 序列读取、方向矫正 |
| RT Structure | GDCM | RT Structure DICOM 文件解析 |
| 语言 | C++17 | 全项目 |

---

## 2. 架构设计

项目采用分层架构，自上而下分为四层：UI 层、Controller 层、Renderer 层、Data & Utils 层。各层之间通过接口（Interface）解耦，遵循依赖倒置原则。

### 2.1 UI 层

负责用户界面展示和交互事件的发起。

- **DicomImageStudy**：主窗口（QMainWindow），承载所有子组件的布局
- **TitleBarWidget**：顶部工具栏，包含文件打开、测量模式切换等按钮，通过 Qt 信号（signal）向下传递用户操作
- **LeftToolWidget**：左侧面板，预留功能扩展区域
- **QVTKOpenGLNativeWidget ×3**：三个 VTK 渲染窗口，分别显示 Axial、Sagittal、Coronal 视图

UI 层不包含任何业务逻辑，仅通过 Qt 的 signal/slot 机制与 Controller 层通信。

### 2.2 Controller 层

核心控制逻辑所在，负责协调用户交互与渲染更新。

#### ThreeViewController

三视图控制器，是整个应用的核心协调者。职责包括：

- 管理三个 IViewRenderer 实例
- 切片导航（滚轮滚动、点击联动）
- 窗宽窗位（WW/WL）调节
- 交互模式切换
- RT Structure 数据加载与分发

所有切片变化统一经过 `updateSliceInternal()` 方法，确保 Overlay（轮廓、十字线等）在任何切片变化场景下都自动更新。

#### Strategy 模式（交互策略）

采用策略模式处理不同的用户交互模式，所有策略实现 `IInteractionStrategy` 接口：

- **NormalStrategy**：默认模式，处理点击联动、滚轮滚动、缩放
- **DistanceMeasureStrategy**：距离测量模式，两点定位画测距线
- **AngleMeasureStrategy**：角度测量模式，三点定位量角器
- **RegistrationROIStrategy**：配准 ROI 选择模式

通过 `InteractionStrategyFactory` 工厂创建，`ThreeViewController::SetInteractionMode()` 切换当前活跃策略。滚轮事件始终由 NormalStrategy 处理（切片滚动不受模式影响）。

### 2.3 Renderer 层

负责图像渲染和 Overlay 叠加显示。

#### VtkViewRenderer

每个视图对应一个 VtkViewRenderer 实例，实现 `IViewRenderer` 接口。职责：

- 管理 vtkImageViewer2（CT 图像显示）
- 管理 Overlay Renderer（叠加层，与主图像共享相机）
- 事件回调注册与分发
- 渲染请求合并（QTimer 防抖，16ms 内多次请求只渲染一次）

采用双层 Renderer 架构：Layer 0 显示 CT 图像，Layer 1（Overlay Renderer）显示十字线、测量标注、轮廓等叠加元素。两层共享同一个相机，确保空间对齐。

#### Overlay 系统

采用组合模式，`IOverlayManager` 管理多个 `IOverlayFeature`：

**IOverlayManager（SimpleOverlayManager）**
- 统一管理所有 Overlay Feature 的生命周期
- 在切片变化时通知所有 Feature 更新
- 提供 `GetFeature<T>()` 模板方法获取特定 Feature
- 提供 `SetRTStructureData()` 接口传递 RT Structure 数据

**IOverlayFeature 实现：**

| Feature | 功能 |
|---------|------|
| SimpleCrosshairManager | 十字线，跟随点击位置，三视图联动 |
| SimpleDistanceMeasureManager | 距离测量，支持多组测量、端点拖拽编辑 |
| SimpleAngleMeasureManager | 角度测量，三点定位，支持编辑 |
| SimpleContourOverlayManager | RT Structure 轮廓叠加显示 |
| SimpleOverlayInfoManager | 视图信息叠加（WW/WL、切片号、坐标） |

**OverlayFactory**：工厂类，创建默认的 Overlay 配置，注册所有 Feature。

### 2.4 Data & Utils 层

负责医学影像数据的读取、转换和写出。

- **CTViewer**：DICOM CT 序列读取，ITK → VTK 转换，方向矩阵处理（Reslice）
- **RtStructReader**：DICOM RT Structure 文件解析（GDCM），提取 ROI 名称、颜色、轮廓坐标
- **ImageOrientationResampler**：图像方向变换与重采样，支持体位校正
- **DicomRegWriter**：DICOM Registration 文件写出

**Common 数据结构：**
- `ViewType`：视图类型枚举（Axial=0, Sagittal=1, Coronal=2）
- `EventData`：交互事件数据（鼠标位置、键盘状态）
- `InteractionMode`：交互模式枚举
- `RenderViewState`：渲染视图状态（WW/WL、切片号、坐标）
- `RTStructureData`：RT Structure 数据结构（ROI 名称、颜色、轮廓点坐标）

---

## 3. 核心流程

### 3.1 图像加载流程

1. 用户点击"打开文件夹"按钮
2. CTViewer 使用 ITK 读取 DICOM 序列，筛选 Modality=CT 的文件
3. ITK 图像转换为 VTK 图像（ITK→VTK Filter）
4. 应用方向矩阵（vtkImageReslice）校正体位
5. ThreeViewController 将 VTK 图像分发给三个 VtkViewRenderer
6. 每个 Renderer 设置对应方向（XY/YZ/XZ），初始化到中间切片

### 3.2 三视图联动流程

1. 用户在任一视图上点击
2. NormalStrategy 接收 LeftPress 事件，获取点击位置的世界坐标
3. 调用 ThreeViewController::UpdateSliceInternals()
4. 世界坐标转换为三个方向的切片索引（ijk）
5. 分别调用 updateSliceInternal() 更新三个视图的切片
6. 每个视图的 OnSliceChanged() 被触发，更新所有 Overlay（十字线、轮廓等）

### 3.3 RT Structure 轮廓显示流程

1. 用户加载 RT Structure 文件
2. RtStructReader 解析 DICOM RS 文件，提取所有 ROI 的轮廓数据
3. 通过 IOverlayManager::SetRTStructureData() 将数据传递给 SimpleContourOverlayManager
4. 每次切片变化时，SimpleContourOverlayManager::OnSliceChanged() 被调用
5. 根据视图类型执行不同的绘制逻辑：
   - **Axial**：匹配 Z 坐标，直接用原始轮廓点画闭合折线（平滑无锯齿）
   - **Sagittal**：用 X=sliceX 平面切割每条轮廓多边形，计算交点，每层取最外侧 left/right 点，纵向连接成边界线
   - **Coronal**：用 Y=sliceY 平面切割，同 Sagittal 逻辑

### 3.4 测量工具流程

1. 用户在工具栏切换到测量模式
2. TitleBarWidget 发出信号，ThreeViewController 切换 InteractionMode
3. 对应 Strategy 接管鼠标事件
4. 点击时通过 VtkViewRenderer::PickWorldPosition() 获取世界坐标
5. Strategy 调用对应的 OverlayFeature 绘制标注
6. 切片变化时，测量标注的坐标随切片同步更新

---

## 4. 设计模式总结

| 模式 | 应用位置 | 说明 |
|------|---------|------|
| 策略模式 | Controller/Strategy | 不同交互模式的行为封装 |
| 工厂模式 | OverlayFactory, InteractionStrategyFactory | 创建和组装组件 |
| 组合模式 | IOverlayManager + IOverlayFeature | 管理多个独立的叠加功能 |
| 观察者模式 | Qt signal/slot, OnSliceChanged | 事件通知与响应 |
| 模板方法 | GetFeature&lt;T&gt;() | 类型安全的组件获取 |

---

## 5. 接口依赖关系

```
ThreeViewController
  ├── IViewRenderer (×3)           // 不依赖具体 VtkViewRenderer
  ├── IInteractionStrategy (map)   // 不依赖具体 Strategy 实现
  └── IOverlayManager              // 不依赖具体 Feature 实现
       └── IOverlayFeature (vector) // 不依赖具体 Manager 实现
```

Controller 层只依赖接口，不依赖 Renderer 层的具体实现类。数据传递（如 RT Structure 数据）通过 IOverlayManager 的通用接口完成，Controller 不需要知道哪个 Feature 在操作数据。

<img width="940" height="1327" alt="image" src="https://github.com/user-attachments/assets/b912867f-0fa0-499e-b4d1-eb1e7e465218" />

