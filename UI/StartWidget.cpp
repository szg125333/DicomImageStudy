#include "StartWidget.h"
#include <QToolBar>
#include <QAction>
#include <QVBoxLayout>

StartWidget::StartWidget(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    // 创建工具栏
    QToolBar* toolBar = new QToolBar("Main Toolbar", this);

    // 按照你图里那几个 action 来添加
    QAction* fileAction = new QAction("File", this);
    QAction* editAction = new QAction("Edit", this);
    QAction* viewAction = new QAction("View", this);
    QAction* windowAction = new QAction("Window", this);
    QAction* helpAction = new QAction("Help", this);

    toolBar->addAction(fileAction);
    toolBar->addAction(editAction);
    toolBar->addAction(viewAction);
    toolBar->addAction(windowAction);
    toolBar->addAction(helpAction);

    // 将工具栏放到主布局最上方
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(toolBar);
    mainLayout->addStretch(); // 后续可以插入影像控件等

    setLayout(mainLayout);
}

StartWidget::~StartWidget()
{
}
