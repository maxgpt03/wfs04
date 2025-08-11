#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <cstdint>
#include <string>
#include <memory>

#include "IFile.h"

class WinFile : public IFile {
public:
	WinFile();
	~WinFile();

	bool open(const std::string& inFilePath) override;
	bool setPosition(uint64_t ui64Offset, uint8_t ui8MoveMethod) override;
	bool read(uint8_t* ui8Buffer, uint32_t ui32Size, uint32_t& ui32BytesRead) override;
	bool writeToFile(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pUi8Data, size_t inDataSize) override;
	bool writeToFileAppend(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pUi8Data, size_t inDataSize) override;
	void close() override;	

private:
	HANDLE fileHandle;
	std::wstring utf8ToWide(const std::string& utf8Str);
};
#endif
