#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindowClass) {
	ui->setupUi(this);

	setWindowTitle(QCoreApplication::translate("MainWindowClass", "WFS Viewer - Просмотр видеофрагментов", nullptr));
	setWindowIcon(QIcon(":/MainWindow/icon.png"));

	ui->actionOpen->setShortcut(QKeySequence("Ctrl+O"));
	connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::onOpenFile);

	ui->treeWidget->setColumnCount(5);
	ui->treeWidget->setHeaderLabels(QStringList() << "Index Fragment" << "Time start" << "Time end" << "Camera number" << "Set poit Frag");
	ui->treeWidget->clear();
	ui->treeWidget->setColumnWidth(0, 95);
	ui->treeWidget->setColumnWidth(1, 120);
	ui->treeWidget->setColumnWidth(2, 120);
	ui->treeWidget->setColumnWidth(3, 50);
	ui->treeWidget->setColumnWidth(4, 60);
	ui->treeWidget->header()->setStretchLastSection(false);
	ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->treeWidget->setSortingEnabled(false);
	connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &MainWindow::onTreeWidgetContextMenu);
	connect(ui->treeWidget, &QTreeWidget::itemExpanded, this, &MainWindow::onItemExpanded);
	connect(ui->treeWidget, &QTreeWidget::itemCollapsed, this, &MainWindow::onItemCollapsed);

	ui->cbCameras->setEnabled(false);

	ui->dateFrom->setEnabled(false);
	ui->dateFrom->setCalendarPopup(true);
	ui->dateFrom->setDate(QDate::currentDate().addDays(-1));

	ui->dateTo->setEnabled(false);
	ui->dateTo->setCalendarPopup(true);
	ui->dateTo->setDate(QDate::currentDate());

	ui->btnApplyFilter->setEnabled(false);
	connect(ui->btnApplyFilter, &QPushButton::clicked, this, &MainWindow::applyFilter);

	ui->btnResetFilter->setEnabled(false);
	connect(ui->btnResetFilter, &QPushButton::clicked, this, &MainWindow::resetFilter);

	connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::openAboutWindow);
}

MainWindow::~MainWindow() {
	delete ui;
}

void MainWindow::onOpenFile() {
	ui->treeWidget->clear();
	QString fileName = QFileDialog::getOpenFileName(this, tr("Открыть файл"), "", tr("Все файлы (*.*)"));

	if (fileName.isEmpty()) {
		QMessageBox::critical(this, "Ошибка", "Файл не задан.");
		return;
	}
	someWFS.reset();

	file = createPlatformFile();

	std::string fileStr = fileName.toUtf8().constData();

	if (!file->open(fileStr)) {
		QMessageBox::critical(this, "Ошибка", "Не удалось открыть файл.");
		return;
	}

	someWFS = std::make_unique<FileSystem_WFS>(std::move(file));
	ui->statusBar->showMessage("Файл открыт: " + fileName, 6000);

	ui->treeWidget->setSortingEnabled(false);
	uint16_t ui16CameraCount = 0;
	for (auto iterFragChain = someWFS->mapValidChains.begin(); iterFragChain != someWFS->mapValidChains.end(); ++iterFragChain) {
		uint32_t ui32IndexCurrentMainDesc = iterFragChain->first;
		FragmentChain& videoChainCurMainDesc = iterFragChain->second;
		uint16_t ui16CountSecDesc = videoChainCurMainDesc.pMainDes->ui16CountSecDesc;

		// Переменная ui16CameraCount для QComboBox* cbCameras
		if (ui16CameraCount < videoChainCurMainDesc.pMainDes->ui8CameraNumber) {
			ui16CameraCount = videoChainCurMainDesc.pMainDes->ui8CameraNumber;
		}

		MyTreeWidgetItem* item = new MyTreeWidgetItem(ui->treeWidget);

		item->setText(0, QString::number(ui32IndexCurrentMainDesc));
		item->setData(0, Qt::UserRole, ui32IndexCurrentMainDesc);

		item->setText(1, formatTimestamp(videoChainCurMainDesc.pMainDes->stTimeStampStartVideoStream));
		item->setData(1, Qt::UserRole, toQDateTime(videoChainCurMainDesc.pMainDes->stTimeStampStartVideoStream));

		item->setText(2, formatTimestamp(videoChainCurMainDesc.pMainDes->stTimeStampEndVideoStream));
		item->setData(2, Qt::UserRole, toQDateTime(videoChainCurMainDesc.pMainDes->stTimeStampEndVideoStream));

		item->setText(3, QString::number(videoChainCurMainDesc.pMainDes->ui8CameraNumber));
		item->setData(3, Qt::UserRole, videoChainCurMainDesc.pMainDes->ui8CameraNumber);

		item->setText(4, "+");
		item->setData(4, Qt::UserRole, QVariant::fromValue(reinterpret_cast<void*>(&(iterFragChain->second))));

		for (uint16_t ui16Iter = 0; ui16Iter < ui16CountSecDesc; ui16Iter++) {
			MyTreeWidgetItem* child = new MyTreeWidgetItem();
			auto iterSecDesc = videoChainCurMainDesc.pSecDes.find(ui16Iter);
			if (iterSecDesc != videoChainCurMainDesc.pSecDes.end()) {
				/*
				* Дочерние элементы можно инициализировать при раскрытии выпадающего списка
				* Если инициализировать их в данном месте, то будет
				* Если этого не делать, то только при инициализации MainDesc = 384 Мб
				* MainDesc + SecDesc = 624 Мб
				* Экономия = 240 Мб
				* video.mkv
				*/

				/*
				child->setText(0, QString::number(iterSecDesc->first));
				child->setText(1, formatTimestamp(iterSecDesc->second->stTimeStampStartVideoSegment));
				child->setText(2, formatTimestamp(iterSecDesc->second->stTimeStampEndVideoSegment));
				child->setText(3, QString::number(iterSecDesc->second->ui8CameraNumber));
				*/
			}
			else {
				//std::cout << "\t[" << ui16Iter << "] - X" << std::endl;
				child->setText(0, QString::number(ui16Iter));
				child->setData(0, Qt::UserRole, ui16Iter);

				child->setText(1, "-");
				child->setText(2, "-");
				child->setText(3, "-");
			}
			item->addChild(child);
		}
		ui->treeWidget->addTopLevelItem(item);
	}

	/*
	* Добавление в QTreeWidget восстановленных цепочек
	*/
	for (auto iterFragChain = someWFS->mapIncompleteChains.begin(); iterFragChain != someWFS->mapIncompleteChains.end(); ++iterFragChain) {
		uint32_t ui32IndexCurrentMainDesc = iterFragChain->first;
		FragmentChain& videoChainCurMainDesc = iterFragChain->second;

		uint16_t ui16CountSecDesc = videoChainCurMainDesc.pMainDes->ui16CountSecDesc;

		MyTreeWidgetItem* item = new MyTreeWidgetItem(ui->treeWidget);
		item->setText(0, QString::number(ui32IndexCurrentMainDesc));
		item->setData(0, Qt::UserRole, ui32IndexCurrentMainDesc);

		item->setText(1, formatTimestamp(videoChainCurMainDesc.pMainDes->stTimeStampStartVideoStream));
		item->setData(1, Qt::UserRole, toQDateTime(videoChainCurMainDesc.pMainDes->stTimeStampStartVideoStream));

		item->setText(2, formatTimestamp(videoChainCurMainDesc.pMainDes->stTimeStampEndVideoStream));
		item->setData(2, Qt::UserRole, toQDateTime(videoChainCurMainDesc.pMainDes->stTimeStampEndVideoStream));

		item->setText(3, QString::number(videoChainCurMainDesc.pMainDes->ui8CameraNumber));
		item->setData(3, Qt::UserRole, videoChainCurMainDesc.pMainDes->ui8CameraNumber);

		item->setText(4, "+");
		item->setData(4, Qt::UserRole, QVariant::fromValue(reinterpret_cast<void*>(&(iterFragChain->second))));
		
		for (int i = 0; i < ui->treeWidget->columnCount(); ++i) {
			item->setBackground(i, QBrush(QColor("#f2ca16")));
		}

		for (uint16_t ui16Iter = 0; ui16Iter < ui16CountSecDesc; ui16Iter++) {
			MyTreeWidgetItem* child = new MyTreeWidgetItem();
			auto iterSecDesc = videoChainCurMainDesc.pSecDes.find(ui16Iter);
			if (iterSecDesc != videoChainCurMainDesc.pSecDes.end()) {

			}
			else {
				child->setText(0, QString::number(ui16Iter));
				child->setData(0, Qt::UserRole, ui16Iter);

				child->setText(1, "-");
				child->setText(2, "-");
				child->setText(3, "-");
			}
			item->addChild(child);
		}

		ui->treeWidget->addTopLevelItem(item);
	}
	for (int i = 0; i < ui->treeWidget->columnCount(); ++i) {
		ui->treeWidget->resizeColumnToContents(i);
	}

	// Работа с графическими элементами фильтрации
	ui->cbCameras->setEnabled(true);
	ui->dateFrom->setEnabled(true);
	ui->dateTo->setEnabled(true);
	ui->btnApplyFilter->setEnabled(true);
	ui->btnResetFilter->setEnabled(true);

	ui->cbCameras->addItem("Все камеры");
	if (ui16CameraCount > 0) {
		for (uint16_t ui16Iter = 1; ui16Iter <= ui16CameraCount; ui16Iter++) {
			ui->cbCameras->addItem(QString::number(ui16Iter));
		}		
	}
	ui->treeWidget->header()->setSortIndicator(-1, Qt::AscendingOrder);
	ui->treeWidget->setSortingEnabled(true);
}

void MainWindow::onTreeWidgetContextMenu(const QPoint& pos) {
	MyTreeWidgetItem* item = dynamic_cast<MyTreeWidgetItem*>(ui->treeWidget->itemAt(pos));
	//MyTreeWidgetItem* item = ui->treeWidget->itemAt(pos);
	if (!item) {
		return;
	}

	QMenu contextMenu(this);

	QAction* qActionInfo = contextMenu.addAction("Показать информацию");
	QAction* qActionDelete = contextMenu.addAction("Удалить элемент");
	QAction* qActionSaveVideo;
	if (!item->parent()) {
		qActionSaveVideo = contextMenu.addAction("Сохранить видео цепочки");
	}
	else {
		qActionSaveVideo = contextMenu.addAction("Сохранить видео фрагмента");
	}

	QAction* selectedAction = contextMenu.exec(ui->treeWidget->viewport()->mapToGlobal(pos));
	if (!selectedAction) {
		return;
	}

	if (selectedAction == qActionInfo) {
		QString info = QString("Вы выбрали: \"%1\"").arg(item->text(0));
		QMessageBox::information(this, "Информация", info);
	}
	else if (selectedAction == qActionDelete) {
		delete item;
	}
	else if (selectedAction == qActionSaveVideo) {
		QString text = item->text(4);
		if (text != "+")
			return;

		if (!item->parent()) {
			// Корневой элемент
			QVariant var = item->data(4, Qt::UserRole);
			FragmentChain* pFragmentChain = reinterpret_cast<FragmentChain*>(var.value<void*>());

			QString qSDefaultFileName = QDir::homePath() + "/chain_main_desc_" + QString::number(pFragmentChain->pMainDes->ui32IndexCurrentMainDesc) + ".dav";
			QString qSFileName = QFileDialog::getSaveFileName(this, tr("Сохранить файл"), qSDefaultFileName, tr("Все файлы (*)"));

			if (!qSFileName.isEmpty()) {
				std::string strFileName = qSFileName.toUtf8().constData();
				someWFS->saveVideoChain(*pFragmentChain, strFileName);
				QMessageBox::information(this, "Информация", "Сохранен файл: " + qSFileName);
			}
		}
		else {
			// Дочерний элемент
			QVariant var = item->data(4, Qt::UserRole);
			WFSSecDescAdvInfo* pSecDesc = reinterpret_cast<WFSSecDescAdvInfo*>(var.value<void*>());

			QString qSDefaultFileName = QDir::homePath() + "/main_desc_" + QString::number(pSecDesc->ui32IndexMainDesc) + "_sec_desc_" + QString::number(pSecDesc->ui32IndexCurrentSecDesc) + ".dav";
			QString qSFileName = QFileDialog::getSaveFileName(this, tr("Сохранить файл"), qSDefaultFileName, tr("Все файлы (*)"));

			if (!qSFileName.isEmpty()) {
				std::string strFileName = qSFileName.toUtf8().constData();
				someWFS->saveSecFragmentVideo(*pSecDesc, strFileName);
				QMessageBox::information(this, "Информация", "Сохранен файл: " + qSFileName);	
			}
		}
	}
}

void MainWindow::onItemExpanded(QTreeWidgetItem* item) {
	ui->treeWidget->setSortingEnabled(false);
	uint32_t ui32Index = item->text(0).toUInt();
	QColor bgColor = item->background(0).color();
	int iCount = item->childCount();

	if (bgColor == QColor("#f2ca16")) {
		auto fragmentChain = someWFS->mapIncompleteChains.find(ui32Index);

		if (fragmentChain != someWFS->mapIncompleteChains.end()) {
			uint16_t ui16CountSecDesc = fragmentChain->second.pMainDes->ui16CountSecDesc;

			for (int intIter = 0; intIter < iCount; ++intIter) {
				uint16_t ui16Iter = static_cast<uint16_t>(intIter);
				auto iterSecDesc = fragmentChain->second.pSecDes.find(ui16Iter);
				MyTreeWidgetItem* child = dynamic_cast<MyTreeWidgetItem*>(item->child(intIter));
				//MyTreeWidgetItem* child = item->child(intIter);
				if (iterSecDesc != fragmentChain->second.pSecDes.end()) {
					if (iterSecDesc->second->bIsRecovered) {
						for (int i = 0; i < ui->treeWidget->columnCount(); ++i) {
							child->setBackground(i, QBrush(QColor("#f2ca16")));
						}
					}
					child->setText(0, QString::number(iterSecDesc->first) + " " + QString::number(iterSecDesc->second->ui32IndexCurrentSecDesc));
					child->setData(0, Qt::UserRole, QString::number(iterSecDesc->first));

					child->setText(1, formatTimestamp(iterSecDesc->second->stTimeStampStartVideoSegment));
					child->setData(1, Qt::UserRole, toQDateTime(iterSecDesc->second->stTimeStampStartVideoSegment));

					child->setText(2, formatTimestamp(iterSecDesc->second->stTimeStampEndVideoSegment));
					child->setData(2, Qt::UserRole, toQDateTime(iterSecDesc->second->stTimeStampEndVideoSegment));

					child->setText(3, QString::number(iterSecDesc->second->ui8CameraNumber));
					child->setData(3, Qt::UserRole, iterSecDesc->second->ui8CameraNumber);

					child->setText(4, "+");
					child->setData(4, Qt::UserRole, QVariant::fromValue(reinterpret_cast<void*>(iterSecDesc->second)));
				}
				else {
					child->setText(0, QString::number(ui16Iter));
					child->setData(0, Qt::UserRole, ui16Iter);

					child->setText(1, "-");
					child->setText(2, "-");
					child->setText(3, "-");
					child->setText(4, "-");
				}
			}
		}
	}
	else {
		auto fragmentChain = someWFS->mapValidChains.find(ui32Index);

		if (fragmentChain != someWFS->mapValidChains.end()) {
			uint16_t ui16CountSecDesc = fragmentChain->second.pMainDes->ui16CountSecDesc;

			for (int intIter = 0; intIter < iCount; ++intIter) {
				uint16_t ui16Iter = static_cast<uint16_t>(intIter);
				auto iterSecDesc = fragmentChain->second.pSecDes.find(ui16Iter);
				MyTreeWidgetItem* child = dynamic_cast<MyTreeWidgetItem*>(item->child(intIter));

				//MyTreeWidgetItem* child = item->child(intIter);
				if (iterSecDesc != fragmentChain->second.pSecDes.end()) {
					if (iterSecDesc->second->bIsRecovered) {
						for (int i = 0; i < ui->treeWidget->columnCount(); ++i) {
							child->setBackground(i, QBrush(QColor("#f2ca16")));
						}
					}
					child->setText(0, QString::number(iterSecDesc->first) + " " + QString::number(iterSecDesc->second->ui32IndexCurrentSecDesc));
					child->setData(0, Qt::UserRole, iterSecDesc->first);

					child->setText(1, formatTimestamp(iterSecDesc->second->stTimeStampStartVideoSegment));
					child->setData(1, Qt::UserRole, toQDateTime(iterSecDesc->second->stTimeStampStartVideoSegment));

					child->setText(2, formatTimestamp(iterSecDesc->second->stTimeStampEndVideoSegment));
					child->setData(2, Qt::UserRole, toQDateTime(iterSecDesc->second->stTimeStampEndVideoSegment));

					child->setText(3, QString::number(iterSecDesc->second->ui8CameraNumber));

					child->setText(4, "+");
					child->setData(4, Qt::UserRole, QVariant::fromValue(reinterpret_cast<void*>(iterSecDesc->second)));
				}
				else {
					child->setText(0, QString::number(ui16Iter));
					child->setData(0, Qt::UserRole, ui16Iter);

					child->setText(1, "-");
					child->setText(2, "-");
					child->setText(3, "-");
					child->setText(4, "-");
				}
			}
		}
	}
	for (int i = 0; i < ui->treeWidget->columnCount(); ++i) {
		ui->treeWidget->resizeColumnToContents(i);
	}
	ui->treeWidget->header()->setSortIndicator(-1, Qt::AscendingOrder);
	ui->treeWidget->setSortingEnabled(true);
}

void MainWindow::onItemCollapsed(QTreeWidgetItem* item) {
	//qDebug() << "Свернут элемент:" << item->text(0);
}

void MainWindow::applyFilter() {
	QString selectedCamera = ui->cbCameras->currentText();
	QDate dateStart = ui->dateFrom->date();
	QDate dateEnd = ui->dateTo->date();

	for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
		MyTreeWidgetItem* item = dynamic_cast<MyTreeWidgetItem*>(ui->treeWidget->topLevelItem(i));
		//MyTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
		bool match = true;

		// Фильтр по камере
		if (selectedCamera != "Все камеры") {
			QString qsCamera = item->text(3); // 3-я колонка — Camera
			if (qsCamera != selectedCamera) {
				match = false;
			}
		}

		// Фильтр по дате (Time start колонка — колонка 1)
		if (match) {
			QDate date = QDate::fromString(item->text(1).split(' ').value(1), "dd.MM.yyyy");
			if (date < dateStart || date > dateEnd) {
				match = false;
			}
		}
		item->setHidden(!match);
	}
}


void MainWindow::resetFilter() {
	for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
		MyTreeWidgetItem* item = dynamic_cast<MyTreeWidgetItem*>(ui->treeWidget->topLevelItem(i));
		//MyTreeWidgetItem* item = ui->treeWidget->topLevelItem(i);
		item->setHidden(false);
	}
}

void MainWindow::openAboutWindow() {
	AboutWindow* about = new AboutWindow(this);
	about->show();
	about->setAttribute(Qt::WA_DeleteOnClose);
}