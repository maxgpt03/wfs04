#pragma once

#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QAction>
#include <QTreeWidget>
#include <QMetaType>
#include <QShortcut>
#include <QKeySequence>
#include <QHeaderView>

#include "ui_MainWindow.h"
#include "core/FileSystem_WFS.h"
#include "core/struct_wfs.h"
#include "io/IFile.h"
#include "io/macFile.h"
#include "io/WinFile.h"
#include "../utils.h"
#include "../MyTreeWidgetItem.h"

#include "AboutWindow.h"

class FileSystem_WFS;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private:
	Ui::MainWindowClass* ui;
	std::unique_ptr<FileSystem_WFS> someWFS;
	std::unique_ptr<IFile> file;

private slots:
	void onOpenFile();
	void onTreeWidgetContextMenu(const QPoint& pos);
	void onItemExpanded(QTreeWidgetItem* item);
	void onItemCollapsed(QTreeWidgetItem* item);
	void applyFilter();
	void resetFilter();
	void openAboutWindow();
};