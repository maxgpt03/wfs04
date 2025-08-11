#pragma once
#include <vector>
#include <list>
#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <map>
#include <chrono>
#include <cstdint>
#include <memory>

#include "struct_wfs.h"
#include "../io/IFile.h"

class FileSystem_WFS
{
public:
	explicit FileSystem_WFS(std::unique_ptr<IFile> inFile);
	std::map<uint32_t, FragmentChain> mapValidChains;				// Ассоциативный контейнер видеофрагментов с MainDesc
	std::map<uint32_t, FragmentChain> mapIncompleteChains;			// Ассоциативный контейнер видеофрагментов без MainDesc

	// Сохраняет цепочку видеофрагментов в файл
	void saveVideoChain(const FragmentChain& inFragmentChain, const std::string& inString);

	// Сохраняет видеофрагмент в файл
	void saveSecFragmentVideo(const WFSSecDescAdvInfo& inSecDesc, const std::string& inString);

private:
	WFSAllValue stWFSAllValue;
	std::unique_ptr<IFile> inputFile_;
	std::map<uint32_t, WFSMainDescAdvInfo> mapMainDesc;		// Ассоциативный контейнер MainDesc
	std::map<uint32_t, WFSSecDescAdvInfo> mapSecDesc;			// Ассоциативный контейнер SecDesc

	// === Анализ и проверка структуры WFS ===
	template <typename T> T readStruct(uint64_t inUi64Offset, uint32_t inUi32Size);
	std::unique_ptr<uint8_t[]> readRawData(uint64_t inUi64Offset, uint32_t inUi32Size);
	bool checkWFSHeader(const WFSHeader& inPStWFSHeader);
	bool checkWFSSuperBlock(const WFSSuperBlock& inPStWFSSuperblock);
	bool isWFS();
	void initSuperBlock();
	void analysisIndexArea();
	void rebuildUnwrittenVideoChain();
	void rebuildOverwrittenVideoChain();

	// === Вспомогательные утилиты ===
	bool isLikelyMainDesc(uint32_t inUi32IndexDesc, uint32_t inUi32SizeDescVideoFragment, const void* inPoitCurrentPosition);
	bool isLikelySecDesc(uint32_t inUi32SizeDescVideoFragment, const void* inPoitCurrentPosition);
	WFSDateTime convertTime(uint32_t inU32TimeValue);
	bool isValidDateTime(const WFSDateTime& inStDateWFS);
	
	// === Вывод информации ===
	void printWFSInf();
	void printWFSDateTime(const WFSDateTime& inStDateWFS);
	void printValidChains(FragmentChain& inFragmentChain);
	void printIncompleteChains(FragmentChain& inFragmentChain);
	void printAllChains();
	void dumpHex(const void* vPointOffsetPrintData, uint32_t ui32SizePrintData, uint64_t ui64OffsetInFWS);
};