#pragma once

#include <QWidget>
#include "ui_StartWidget.h"

class StartWidget : public QWidget
{
	Q_OBJECT

public:
	StartWidget(QWidget *parent = nullptr);
	~StartWidget();

private:
	Ui::StartWidgetClass ui;
};

