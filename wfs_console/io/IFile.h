#pragma once
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#define FILE_ORIGIN_BEGIN FILE_BEGIN
#define FILE_ORIGIN_CUR   FILE_CURRENT
#define FILE_ORIGIN_END   FILE_END
#else
#include <unistd.h>
#define FILE_ORIGIN_BEGIN SEEK_SET
#define FILE_ORIGIN_CUR   SEEK_CUR
#define FILE_ORIGIN_END   SEEK_END
#endif

class IFile {
public:
	virtual ~IFile() = default;

	virtual bool open(const std::string& inFilePath) = 0;
	virtual bool setPosition(uint64_t ui64Offset, uint8_t ui8MoveMethod = FILE_ORIGIN_BEGIN) = 0;
	virtual bool read(uint8_t* ui8Buffer, uint32_t ui32Size, uint32_t& ui32BytesRead) = 0;
	virtual bool writeToFile(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pData, size_t dataSize) = 0;
	virtual bool writeToFileAppend(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pData, size_t dataSize) = 0;
	virtual void close() = 0;
};