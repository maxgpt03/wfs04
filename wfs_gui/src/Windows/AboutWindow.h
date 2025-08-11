#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QFile>

#include "ui_AboutWindow.h"

namespace Ui {
    class AboutWindow;
}

class AboutWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit AboutWindow(QWidget* parent = nullptr);
    ~AboutWindow();

private:
    Ui::AboutWindow* ui;
};