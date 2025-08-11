#pragma once
#if defined(__MACH__) && defined(__APPLE__)
#include <iostream>
#include <fstream>
#include <string>
#include <locale>

#include "IFile.h"


class macFile : public IFile {
public:
	macFile();
	~macFile();

	bool open(const std::string& inFilePath) override;
	bool setPosition(uint64_t ui64Offset, uint8_t ui8MoveMethod) override;
	bool read(uint8_t* ui8Buffer, uint32_t ui32Size, uint32_t& ui32BytesRead) override;
	bool writeToFile(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pData, size_t dataSize) override;
	bool writeToFileAppend(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pData, size_t dataSize) override;
	void close() override;

private:
	std::ifstream inputFile_;
};
#endif