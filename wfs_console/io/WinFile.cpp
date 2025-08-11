#ifdef _WIN32
#include "WinFile.h"

WinFile::WinFile() : fileHandle(INVALID_HANDLE_VALUE) {};

WinFile::~WinFile() {
	close();
};

bool WinFile::open(const std::string& inFilePath) {
	uint32_t ui32ResulGetLastErrorCode = NO_ERROR;

	std::wstring widePath = utf8ToWide(inFilePath);
	if (widePath.empty()) return false;

	fileHandle = CreateFileW(
		widePath.c_str(),						// Дескпритор файла
		GENERIC_READ,							// режим доступа
		FILE_SHARE_READ,						// Указание возможен ли одновременный доступ
		NULL,									// Атрибуты безопасности
		OPEN_EXISTING,							// Способ открыти¤
		FILE_ATTRIBUTE_NORMAL,					// Флаги и атрибуты
		NULL									// Использование шаблона
	);

	if (fileHandle == INVALID_HANDLE_VALUE) {
		ui32ResulGetLastErrorCode = GetLastError();
		if (ui32ResulGetLastErrorCode == ERROR_FILE_NOT_FOUND) {
		
		}
		if (ui32ResulGetLastErrorCode == ERROR_ALREADY_EXISTS) {

		}
		if (ui32ResulGetLastErrorCode == ERROR_PATH_NOT_FOUND) {

		}
		if (ui32ResulGetLastErrorCode == ERROR_ACCESS_DENIED) {

		}
		return false;
	}
	else {
		return true;
	}
};

bool WinFile::setPosition(uint64_t ui64Offset, uint8_t ui8MoveMethod) {
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	uint32_t ui32ResulGetLastErrorCode = NO_ERROR;
	LARGE_INTEGER liOffsetHeader;

	liOffsetHeader.QuadPart = ui64Offset;

	uint32_t ui32ResultSetPosition = SetFilePointer(fileHandle, liOffsetHeader.LowPart, &liOffsetHeader.HighPart, ui8MoveMethod);
	ui32ResulGetLastErrorCode = GetLastError();
	if (ui32ResultSetPosition == INVALID_SET_FILE_POINTER && ui32ResulGetLastErrorCode != NO_ERROR) {
		return false;
	}
	return true;
};

bool WinFile::read(uint8_t* ui8Buffer, uint32_t ui32Size, uint32_t& ui32BytesRead) {
	if (fileHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	DWORD dwBytesRead = 0;

	bool blResultRead = ReadFile(fileHandle, ui8Buffer, ui32Size, &dwBytesRead, NULL);
	if ((!blResultRead) || (ui32Size != dwBytesRead)) {
		return false;
	}
	ui32BytesRead = static_cast<uint32_t>(dwBytesRead);
	return true;
};

void WinFile::close() {
	if (fileHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(fileHandle);
		fileHandle = INVALID_HANDLE_VALUE;
	}
};

bool WinFile::writeToFile(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pUi8Data, size_t inDataSize) {
	DWORD ui32ResulGetLastErrorCode = NO_ERROR;

	std::wstring widePath = utf8ToWide(inFilePath);
	if (widePath.empty()) return false;

	HANDLE hFile = CreateFileW(
		widePath.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		ui32ResulGetLastErrorCode = GetLastError();
		if (ui32ResulGetLastErrorCode == ERROR_ALREADY_EXISTS) {

		}
		if (ui32ResulGetLastErrorCode == ERROR_PATH_NOT_FOUND) {

		}
		if (ui32ResulGetLastErrorCode == ERROR_ACCESS_DENIED) {

		}
		return false;
	}

	DWORD dwNumberOfBytesToWrite = 0;
	BOOL bResult = WriteFile(
		hFile,
		pUi8Data.get(),
		static_cast<DWORD>(inDataSize),
		&dwNumberOfBytesToWrite,
		nullptr
	);

	if (!bResult || dwNumberOfBytesToWrite != inDataSize) {
		CloseHandle(hFile);
		return false;
	}

	CloseHandle(hFile);
	return true;
};

bool WinFile::writeToFileAppend(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pUi8Data, size_t inDataSize) {
	DWORD ui32ResulGetLastErrorCode = NO_ERROR;

	std::wstring widePath = utf8ToWide(inFilePath);
	if (widePath.empty()) return false;

	HANDLE hFile = CreateFileW(
		widePath.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		ui32ResulGetLastErrorCode = GetLastError();
		if (ui32ResulGetLastErrorCode == ERROR_ALREADY_EXISTS) {

		}
		if (ui32ResulGetLastErrorCode == ERROR_PATH_NOT_FOUND) {

		}
		if (ui32ResulGetLastErrorCode == ERROR_ACCESS_DENIED) {

		}
		return false;
	}

	uint32_t ui32ResultSetPosition = SetFilePointer(hFile, 0, nullptr, FILE_END);
	ui32ResulGetLastErrorCode = GetLastError();
	if (ui32ResultSetPosition == INVALID_SET_FILE_POINTER && ui32ResulGetLastErrorCode != NO_ERROR) {
		CloseHandle(hFile);		
		return false;
	}

	DWORD dwNumberOfBytesToWrite = 0;
	BOOL bResult = WriteFile(
		hFile,
		pUi8Data.get(),
		static_cast<DWORD>(inDataSize),
		&dwNumberOfBytesToWrite,
		nullptr
	);

	if (!bResult || dwNumberOfBytesToWrite != inDataSize) {
		CloseHandle(hFile);
		return false;
	}

	CloseHandle(hFile);
	return true;
};

std::wstring WinFile::utf8ToWide(const std::string& utf8Str) {
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, nullptr, 0);
	if (len == 0) return L"";

	std::wstring result(len, 0);
	if (MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), -1, &result[0], len) == 0) return L"";

	if (!result.empty() && result.back() == L'\0') {
		result.pop_back(); // Удаление \0
	}

	return result;
}
#endif