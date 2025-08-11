#if defined(__MACH__) && defined(__APPLE__)
#include "macFile.h"

macFile::macFile() {};

macFile::~macFile() {
	close();
};

bool macFile::open(const std::string& inFilePath) {
	inputFile_.open(inFilePath, std::ios::in | std::ios::binary);
	if (!inputFile_) {
		return false;
	}
	if (!inputFile_.is_open()) {
		return false;
	}
	if (inputFile_.fail()) {
		return false;
	}
	return true;
};

bool macFile::setPosition(uint64_t ui64Offset, uint8_t ui8MoveMethod) {
	std::ios_base::seekdir direction;
	switch (ui8MoveMethod) {
		case FILE_ORIGIN_BEGIN: direction = std::ios::beg; break;
		case FILE_ORIGIN_CUR: direction = std::ios::cur; break;
		case FILE_ORIGIN_END: direction = std::ios::end; break;
		default: return false;
	}

	inputFile_.seekg(ui64Offset, direction);
	return inputFile_.good();
};

bool macFile::read(uint8_t* ui8Buffer, uint32_t ui32Size, uint32_t& ui32BytesRead) {
	if (!inputFile_.is_open()) {
		ui32BytesRead = 0;
		return false;
	}

	inputFile_.read(reinterpret_cast<char*>(ui8Buffer), ui32Size);
	ui32BytesRead = static_cast<uint32_t>(inputFile_.gcount());

	return inputFile_.bad() ? false : true;
};

void macFile::close() {
	if (inputFile_.is_open()) {
		inputFile_.close();
	}
};

bool macFile::writeToFile(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pUi8Data, size_t inDataSize) {
	if (inDataSize == 0 || !pUi8Data) {
		return false;
	}

	std::ofstream outputFile(inFilePath, std::ios::binary);
	if (!outputFile) {
		return false;
	}

	outputFile.write(reinterpret_cast<const char*>(pUi8Data.get()), inDataSize);
	if (!outputFile) {
		return false;
	}

	outputFile.close();
	return true;
};

bool macFile::writeToFileAppend(const std::string& inFilePath, const std::unique_ptr<uint8_t[]>& pUi8Data, size_t inDataSize) {
	if (inDataSize == 0 || !pUi8Data) {
		return false;
	}

	std::ofstream outputFile(inFilePath, std::ios::binary | std::ios::app);
	if (!outputFile) {
		return false;
	}

	outputFile.write(reinterpret_cast<const char*>(pUi8Data.get()), inDataSize);
	if (!outputFile) {
		return false;
	}

	outputFile.close();
	return true;
};
#endif