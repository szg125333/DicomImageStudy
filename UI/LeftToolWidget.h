#pragma once

#include <QWidget>
#include <QMap>
#include <QString>
#include "ui_LeftToolWidget.h"


class LeftToolWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LeftToolWidget(QWidget* parent = nullptr);
    ~LeftToolWidget();

    // 外部调用：设置 DICOM 元数据
    void SetDicomMetadata(const QMap<QString, QString>& metadata);

private:
    Ui::LeftToolWidgetClass* ui;
};
