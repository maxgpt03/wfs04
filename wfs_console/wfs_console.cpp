#include <iostream>
#include <memory>
#include <string>
#include <locale>
#include <codecvt>

#include "./core/FileSystem_WFS.h"
#if defined(__MACH__) && defined(__APPLE__)
#include "./io/macFile.h"
#elif defined(_WIN32)
#include "./io/WinFile.h"
#endif

void PrintHelp() {
	std::cout << "WFS Console Tool — утилита для работы с файловой системой WFS." << std::endl;
	std::cout << std::endl;
	std::cout << "Использование:" << std::endl;
	std::cout << "    wfs_console <путь_к_образу_WFS>" << std::endl;
	std::cout << std::endl;
	std::cout << "Параметры:" << std::endl;
	std::cout << "    <путь_к_образу_WFS>   Путь к файлу-образу DVR/WFS. Поддерживаются пути в UTF-8." << std::endl;
	std::cout << std::endl;
	std::cout << "Примеры:" << std::endl;
	std::cout << "    wfs_console D:\\images\\wfs.dd" << std::endl;
	std::cout << "    wfs_console /Volumes/DVR/wfs.dd" << std::endl;
	std::cout << std::endl;
	std::cout << "Дополнительно:" << std::endl;
	std::cout << "    Программа кросс-платформенная и работает на Windows, Linux и macOS," << std::endl;
	std::cout << "    включая поддержку архитектур x86_64 и ARM64 (например, Apple Silicon)." << std::endl;
	std::cout << std::endl;
}

int main(int argc, char** argv) {
	setlocale(LC_ALL, "ru_RU.UTF-8");
	std::string stringPath;

	if (argc < 2) {
		PrintHelp();
		return 0;
	}
	if (argc == 2) {
		stringPath = argv[1];
	}

#if defined(__MACH__) && defined(__APPLE__)
	std::unique_ptr<IFile> file = std::make_unique<macFile>();
#elif defined(_WIN32)
	std::unique_ptr<IFile> file = std::make_unique<WinFile>();
#endif

	try {
		if (!file->open(stringPath)) {
			std::cout << "Ошибка чтения файла: " << stringPath << std::endl;
			return 0;
		}
		std::unique_ptr<FileSystem_WFS> someWFS = std::make_unique<FileSystem_WFS>(std::move(file));
	}
	catch (const std::runtime_error& e) {
		std::cout << "Ошибка: " << e.what() << std::endl;
		return 0;
	}
	catch (...) {
		std::cout << "Неизвестная ошибка" << std::endl;
		return 0;
	}

	return 1;
}