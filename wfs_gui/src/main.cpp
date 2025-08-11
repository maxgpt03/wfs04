#include "Windows/MainWindow.h"

int main(int argc, char *argv[]) {
	setlocale(LC_ALL, "ru_RU.UTF-8");
	QApplication app(argc, argv);
	qRegisterMetaType<FragmentChain*>("FragmentChain*");
	MainWindow window;
	window.show();
	return app.exec();
}