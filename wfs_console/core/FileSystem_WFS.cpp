#include "FileSystem_WFS.h"


/**
* \brief
* Конструктор WFS
*
* \param
* std::unique_ptr<IFile> inFile - умный указатель на интерфейс IFile,
* используемый для абстрактной работы с файлами (открытие, чтение, запись, закрытие и др.).
**/
FileSystem_WFS::FileSystem_WFS(std::unique_ptr<IFile> inFile) : inputFile_(std::move(inFile)) {
	auto start = std::chrono::high_resolution_clock::now();
	if (!isWFS()) {
		throw std::runtime_error("FileSystem_WFS::FileSystem_WFS() - Invalid WFS header");
	}

	initSuperBlock();
	analysisIndexArea();
	rebuildUnwrittenVideoChain();
	rebuildOverwrittenVideoChain();
	printWFSInf();

	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;
	std::cout << "Время выполнения: " << elapsed.count() << " секунд" << std::endl;
}

/**
* \brief
* Проверяет, соответствует ли текущие данные файловой системе формата WFS.
* Читает заголовок WFS с нулевого смещения и проверяет его на корректность.
* 
* \return
* Возвращает true, если заголовок проходит проверку, иначе — false.
**/
bool FileSystem_WFS::isWFS() {
	uint64_t ui64OffsetHeader = 0;

	WFSHeader stWFSHeader = readStruct<WFSHeader>(ui64OffsetHeader, sizeof(WFSHeader));
	if (!checkWFSHeader(stWFSHeader)) {
		return false;
	}
	return true;
}

/**
* \brief
* Проверяет наличие сигнатур файловой системы WFS в передаваемой структуре.
* 
* \param
* const WFSHeader& inStWFSHeader - передаваемая структура.
*
* \return
* Возвращает true, если в структуре содержится проверяемое значение, иначе — false.
**/
bool FileSystem_WFS::checkWFSHeader(const WFSHeader& inStWFSHeader) {
	const uint8_t ui8WFSSig04[] = { 0x57, 0x46, 0x53, 0x30, 0x2E, 0x34 };	// WFS0.4
	const uint8_t ui8WFSSig05[] = { 0x57, 0x46, 0x53, 0x30, 0x2E, 0x35 };	// WFS0.5

	const uint8_t ui8WFSEndHeader[] = { 0x58, 0x4D };

	if ((memcmp(inStWFSHeader.ui8Signature, ui8WFSSig04, sizeof(ui8WFSSig04)) == 0) or
		(memcmp(inStWFSHeader.ui8Signature, ui8WFSSig05, sizeof(ui8WFSSig05)) == 0)) {
		if (memcmp(inStWFSHeader.ui8EndHeader, ui8WFSEndHeader, sizeof(ui8WFSEndHeader)) == 0) {
			return true;
		}
	}
	return false;
}

/**
* \brief
* Читает данные из файла по указанному смещению и интерпретирует их как структуру типа T.
* 
* \param
* <typename T> - Тип структуры, в которую будут интерпретированы прочитанные данные.
* 
* uint64_t inUi64Offset - Смещение в файле, с которого начинается чтение.
* 
* uint32_t inUi32Size - Количество байт, которое необходимо прочитать.
* 
* \return
* Объект типа T, заполненный прочитанными данными.
**/
template <typename T> T FileSystem_WFS::readStruct(uint64_t inUi64Offset, uint32_t inUi32Size) {
	if (sizeof(T) > inUi32Size) {
		throw std::runtime_error("FileSystem_WFS::readStruct() - Struct size exceeds buffer");
	}

	if (!inputFile_->setPosition(inUi64Offset, FILE_ORIGIN_BEGIN)) {
		throw std::runtime_error("FileSystem_WFS::readStruct() - Failed to set file position");
	}

	std::unique_ptr<uint8_t[]> uiBuffer(new uint8_t[inUi32Size]);
	uint32_t ui32BytesRead = 0;
	if (!inputFile_->read(uiBuffer.get(), inUi32Size, ui32BytesRead)) {
		throw std::runtime_error("FileSystem_WFS::readStruct() - Failed to read data");
	}

	if (ui32BytesRead != inUi32Size) {
		throw std::runtime_error("FileSystem_WFS::readStruct() - Incomplete read");
	}

	T result;
	std::memcpy(&result, uiBuffer.get(), sizeof(T));
	return result;
}

/**
* \brief
* Читает указанный объём данных из файла, начиная с заданного смещения.
*
* \param
* uint64_t inUi64Offset - Смещение в файле, с которого начинается чтение.
*
* uint32_t inUi32Size - Количество байт, которое необходимо прочитать.
*
* \return
* std::unique_ptr<uint8_t[]> - Указатель на буфер, содержащий прочитанные данные. 
**/
std::unique_ptr<uint8_t[]> FileSystem_WFS::readRawData(uint64_t inUi64Offset, uint32_t inUi32Size) {
	if (!inputFile_->setPosition(inUi64Offset, FILE_ORIGIN_BEGIN)) {
		throw std::runtime_error("FileSystem_WFS::readRawData() - Failed to set file position");
	}

	std::unique_ptr<uint8_t[]> uiBuffer(new uint8_t[inUi32Size]);
	uint32_t ui32BytesRead = 0;
	if (!inputFile_->read(uiBuffer.get(), inUi32Size, ui32BytesRead)) {
		throw std::runtime_error("FileSystem_WFS::readRawData() - Failed to read data");
	}

	if (ui32BytesRead != inUi32Size) {
		throw std::runtime_error("FileSystem_WFS::readRawData() - Incomplete read");
	}

	return uiBuffer;
}

/**
* \brief
* Проверка сигнатуры конца супер блока.
*
* \param
* const WFSSuperBlock& inPStWFSSuperblock - передаваемая структура супер блока.
*
* \return
* Возвращает true, супер блок корректный, иначе — false.
**/
bool FileSystem_WFS::checkWFSSuperBlock(const WFSSuperBlock& inPStWFSSuperblock) {
	const uint8_t ui8WFSuperBlockSSignatures[] = { 0xDE, 0xBC, 0x9A, 0x78 };

	if (memcmp(&inPStWFSSuperblock.ui32SuperBlockSignatures, ui8WFSuperBlockSSignatures, sizeof(ui8WFSuperBlockSSignatures)) == 0)
		return true;
	return false;
}

/**
* \brief
* Инициализация внутренней структуры stWFSAllValue класса FileSystem_WFS на основании данных из супер блока.
**/
void FileSystem_WFS::initSuperBlock() {
	uint32_t ui32SizeDescriptor = sizeof(WFSIndexAreaMainDesc);
	uint64_t ui64OffsetSuperBlock = 0x3000;

	WFSSuperBlock stWFSSuperBlock = readStruct<WFSSuperBlock>(ui64OffsetSuperBlock, sizeof(WFSSuperBlock));
	if (!checkWFSSuperBlock(stWFSSuperBlock)) {
		throw std::runtime_error("FileSystem_WFS::initSuperBlock() - Invalid WFS SuperBlock");
	}

	stWFSAllValue.ui32DiskBlockSize					 = stWFSSuperBlock.ui32DiskBlockSize;
	stWFSAllValue.ui32CountAllVideoFragments		 = stWFSSuperBlock.ui32CountAllVideoFragments;
	stWFSAllValue.ui32ReservedVideoFragmentCount	 = stWFSSuperBlock.ui32ReservedVideoFragmentCount;
	stWFSAllValue.ui32VideoFragmentSizeDBS			 = stWFSSuperBlock.ui32VideoFragmentSizeDBS;
	stWFSAllValue.ui32VideoFragmentSizeByte			 = stWFSAllValue.ui32VideoFragmentSizeDBS * stWFSAllValue.ui32DiskBlockSize;
	stWFSAllValue.ui64TotalVideoFragmentSizeBytes	 = static_cast<uint64_t>(stWFSAllValue.ui32VideoFragmentSizeByte) * static_cast<uint64_t>(stWFSAllValue.ui32CountAllVideoFragments);
	stWFSAllValue.ui64ReservedVideoFragmentSizeBytes = static_cast<uint64_t>(stWFSAllValue.ui32VideoFragmentSizeByte) * static_cast<uint64_t>(stWFSAllValue.ui32ReservedVideoFragmentCount);
	stWFSAllValue.ui64UsedVideoFragmentSizeBytes	 = stWFSAllValue.ui64TotalVideoFragmentSizeBytes - stWFSAllValue.ui64ReservedVideoFragmentSizeBytes;

	stWFSAllValue.stWFSTimeStampLastInDataBlock		 = convertTime(stWFSSuperBlock.ui32TimeStampLastInDataBlock);
	stWFSAllValue.stWFSTimeStampLastWrite			 = convertTime(stWFSSuperBlock.ui32TimeStampLastWrite);
	stWFSAllValue.stWFSTimeStampFistLast			 = convertTime(stWFSSuperBlock.ui32TimeStampFistWillReWrite);
	stWFSAllValue.stWFSTimeStampFistVideo			 = convertTime(stWFSSuperBlock.ui32TimeStampFistVideo);

	stWFSAllValue.ui32IndexAreaPosStart				 = stWFSSuperBlock.ui32IndexAreaPosStart;
	stWFSAllValue.ui64IndexAreaOffset				 = stWFSSuperBlock.ui32DiskBlockSize * stWFSAllValue.ui32IndexAreaPosStart;

	stWFSAllValue.ui64IndexAreaDescSizeReservedByte	 = static_cast<uint64_t>(stWFSAllValue.ui32ReservedVideoFragmentCount) * static_cast<uint64_t>(ui32SizeDescriptor);
	stWFSAllValue.ui64IndexAreaOffsetFirstRecord	 = stWFSAllValue.ui64IndexAreaOffset + stWFSAllValue.ui64IndexAreaDescSizeReservedByte;
	stWFSAllValue.ui64IndexAreaDescrSizeAllByte		 = static_cast<uint64_t>(stWFSAllValue.ui32CountAllVideoFragments) * static_cast<uint64_t>(ui32SizeDescriptor);
	stWFSAllValue.ui64IndexAreaDescSizeUsedByte		 = stWFSAllValue.ui64IndexAreaDescrSizeAllByte - stWFSAllValue.ui64IndexAreaDescSizeReservedByte;
	stWFSAllValue.ui64IndexAreaOffsetLastRecord		 = stWFSAllValue.ui64IndexAreaOffset + stWFSAllValue.ui64IndexAreaDescrSizeAllByte;
	stWFSAllValue.ui64IndexAreaOffsetEnd			 = stWFSAllValue.ui64IndexAreaOffsetLastRecord + static_cast<uint64_t>(ui32SizeDescriptor);
	stWFSAllValue.ui32IndexAreaVideoFragmentPosLastWrite		= stWFSSuperBlock.ui32IndexAreaVideoFragmentPosLastWrite;
	stWFSAllValue.ui64IndexAreaVideoFragmentPosLastWriteOffset	= static_cast<uint64_t>(stWFSSuperBlock.ui32IndexAreaVideoFragmentPosLastWrite) * static_cast<uint64_t>(stWFSAllValue.ui32DiskBlockSize);
	stWFSAllValue.ui32IndexAreaVideoFragmentPosReWrite			= stWFSSuperBlock.ui32IndexAreaVideoFragmentPosReWrite;
	stWFSAllValue.ui64IndexAreaVideoFragmentPosReWriteOffset	= static_cast<uint64_t>(stWFSSuperBlock.ui32IndexAreaVideoFragmentPosReWrite) * static_cast<uint64_t>(stWFSAllValue.ui32DiskBlockSize);

	stWFSAllValue.ui32DataAreaPosStart			= stWFSSuperBlock.ui32DataAreaPosStart;
	stWFSAllValue.ui64DataAreaOffsetStart		= static_cast<uint64_t>(stWFSAllValue.ui32DiskBlockSize) * static_cast<uint64_t>(stWFSAllValue.ui32DataAreaPosStart);
	stWFSAllValue.ui64DataAreaOffsetFirstRecord	= stWFSAllValue.ui64DataAreaOffsetStart + stWFSAllValue.ui64ReservedVideoFragmentSizeBytes;
	stWFSAllValue.ui64DataAreaOffsetLastRecord	= stWFSAllValue.ui64DataAreaOffsetStart + stWFSAllValue.ui64TotalVideoFragmentSizeBytes;
	stWFSAllValue.ui64DataAreaOffsetEnd			= stWFSAllValue.ui64DataAreaOffsetLastRecord + static_cast<uint64_t>(stWFSAllValue.ui32VideoFragmentSizeByte);
}

/**
* \brief
* Вывод информации о файловой системе WFS в консоль.
**/
void FileSystem_WFS::printWFSInf() {
	std::cout << "---------------------------------------------------------------------" << std::endl;
	std::cout << "SuperBlock information" << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;
	printWFSDateTime(stWFSAllValue.stWFSTimeStampFistVideo);
	std::cout << "\t- Временная метка первого видеофрагмента в области данных" << std::endl;
	printWFSDateTime(stWFSAllValue.stWFSTimeStampLastInDataBlock);
	std::cout << "\t- Временная метка последнего видеофрагмента записанного в область данных" << std::endl;
	printWFSDateTime(stWFSAllValue.stWFSTimeStampLastWrite);
	std::cout << "\t- Временная метка последнего записанного видеофрагмента" << std::endl;
	printWFSDateTime(stWFSAllValue.stWFSTimeStampFistLast);
	std::cout << "\t- Временная метка первого видеофрагмента который будет перезаписан" << std::endl;

	printf("0x%010X %13u - Общее количество видеофрагментов\n", stWFSAllValue.ui32CountAllVideoFragments, stWFSAllValue.ui32CountAllVideoFragments);
	printf("0x%010X %13u - Количество зарезервированных видеофрагмента\n", stWFSAllValue.ui32ReservedVideoFragmentCount, stWFSAllValue.ui32ReservedVideoFragmentCount);
	printf("0x%010X %13u - Размер дискового блока (байты)\n", stWFSAllValue.ui32DiskBlockSize, stWFSAllValue.ui32DiskBlockSize);
	printf("0x%010X %13u - Размер видеофрагмента (дисковых блоков DBS)\n", stWFSAllValue.ui32VideoFragmentSizeDBS, stWFSAllValue.ui32VideoFragmentSizeDBS);
	printf("0x%010X %13u - Размер видеофрагмента (байты)\n", stWFSAllValue.ui32VideoFragmentSizeByte, stWFSAllValue.ui32VideoFragmentSizeByte);

	printf("0x%010llX %13llu - Размер всех видеофрагментов (байты)\n", stWFSAllValue.ui64TotalVideoFragmentSizeBytes, stWFSAllValue.ui64TotalVideoFragmentSizeBytes);
	printf("0x%010llX %13llu - Размер используемых видеофрагментов (байты)\n", stWFSAllValue.ui64UsedVideoFragmentSizeBytes, stWFSAllValue.ui64UsedVideoFragmentSizeBytes);
	printf("0x%010llX %13llu - Размер зарезервированных видеофрагментов (байты)\n", stWFSAllValue.ui64ReservedVideoFragmentSizeBytes, stWFSAllValue.ui64ReservedVideoFragmentSizeBytes);
	printf("0x%010llX %13llu - Размер всех видео дескрипторов (байты)\n", stWFSAllValue.ui64IndexAreaDescrSizeAllByte, stWFSAllValue.ui64IndexAreaDescrSizeAllByte);
	printf("0x%010llX %13llu - Размер зарезервированных видео дескрипторов (байты)\n", stWFSAllValue.ui64IndexAreaDescSizeReservedByte, stWFSAllValue.ui64IndexAreaDescSizeReservedByte);
	printf("0x%010llX %13llu - Размер используемых видео дескрипторов (байты)\n", stWFSAllValue.ui64IndexAreaDescSizeUsedByte, stWFSAllValue.ui64IndexAreaDescSizeUsedByte);

	std::cout << "---------------------------------------------------------------------" << std::endl;
	std::cout << "IndexArea information" << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;
	printf("0x%07X %9u - Позиция IndexArea (DBS)\n", stWFSAllValue.ui32IndexAreaPosStart, stWFSAllValue.ui32IndexAreaPosStart);
	printf("0x%07llX %9llu - Позиция IndexArea (байты)\n", stWFSAllValue.ui64IndexAreaOffset, stWFSAllValue.ui64IndexAreaOffset);
	printf("0x%07llX %9llu - Позиция первого дескриптора видеофрагмента (байты)\n", stWFSAllValue.ui64IndexAreaOffsetFirstRecord, stWFSAllValue.ui64IndexAreaOffsetFirstRecord);
	printf("0x%07llX %9llu - Позиция последнего дескриптора видеофрагмента (байты)\n", stWFSAllValue.ui64IndexAreaOffsetLastRecord, stWFSAllValue.ui64IndexAreaOffsetLastRecord);
	printf("0x%07llX %9llu - Позиция конца IndexArea (байты)\n", stWFSAllValue.ui64IndexAreaOffsetEnd, stWFSAllValue.ui64IndexAreaOffsetEnd);
	printf("0x%07X %9u - Позиция в IndexArea последнего дескриптора видеофрагмента записанного оборудованием (DBS)\n", stWFSAllValue.ui32IndexAreaVideoFragmentPosLastWrite, stWFSAllValue.ui32IndexAreaVideoFragmentPosLastWrite);
	printf("0x%07llX %9llu - Позиция в IndexArea последнего дескриптора видеофрагмента записанного оборудованием (байты)\n", stWFSAllValue.ui64IndexAreaVideoFragmentPosLastWriteOffset, stWFSAllValue.ui64IndexAreaVideoFragmentPosLastWriteOffset);
	printf("0x%07X %9u - Позиция в IndexArea первого допустимого дескриптора фрагмента после дескрипторов фрагментов, которые будут перезаписаны (DBS)\n", stWFSAllValue.ui32IndexAreaVideoFragmentPosReWrite, stWFSAllValue.ui32IndexAreaVideoFragmentPosReWrite);
	printf("0x%07llX %9llu - Позиция в IndexArea первого допустимого дескриптора фрагмента после дескрипторов фрагментов, которые будут перезаписаны (байты)\n", stWFSAllValue.ui64IndexAreaVideoFragmentPosReWriteOffset, stWFSAllValue.ui64IndexAreaVideoFragmentPosReWriteOffset);

	std::cout << "---------------------------------------------------------------------" << std::endl;
	std::cout << "Информация о количестве дескрипторов после их проверки:" << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;
	printf("%-6u - Кол-во основных дескрипторов\n", stWFSAllValue.ui32CountMainDesc);
	printf("%-6u - Кол-во вторичных дескрипторов\n", stWFSAllValue.ui32CountSecDesc);
	printf("%-6u - Кол-во зарезервированных дескрипторов\n", stWFSAllValue.ui32CountReservedDesc);
	printf("%-6u - Кол-во других данных в IndexArea\n", stWFSAllValue.ui32CountAnotherDesc);
	printf("%-6u - Кол-во всех дескрипторов в IndexArea\n", stWFSAllValue.ui32CountAllDesc);

	std::cout << "---------------------------------------------------------------------" << std::endl;
	std::cout << "DataArea information" << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;
	printf("0x%010X %13u - Позиция DataArea (DBS)\n", stWFSAllValue.ui32DataAreaPosStart, stWFSAllValue.ui32DataAreaPosStart);
	printf("0x%010llX %13llu - Позиция DataArea (байты)\n", stWFSAllValue.ui64DataAreaOffsetStart, stWFSAllValue.ui64DataAreaOffsetStart);
	printf("0x%010llX %13llu - Позиция первого видеофрагмента (байты)\n", stWFSAllValue.ui64DataAreaOffsetFirstRecord, stWFSAllValue.ui64DataAreaOffsetFirstRecord);
	printf("0x%010llX %13llu - Позиция последнего видеофрагмента (байты)\n", stWFSAllValue.ui64DataAreaOffsetLastRecord, stWFSAllValue.ui64DataAreaOffsetLastRecord);
	printf("0x%010llX %13llu - Позиция конца DataArea (байты)\n", stWFSAllValue.ui64DataAreaOffsetEnd, stWFSAllValue.ui64DataAreaOffsetEnd);
}

/**
* \brief
* Выполняет анализ области IndexArea в файловой системе WFS.
*
* Производит разбор и валидацию дескрипторов видеофрагментов:
* 
* - MainDesc (главный)
* 
* - SecDesc (вторичные)
* 
* - ReservedDesc (зарезервированные)
* 
* - Unknown / Other (неопознанные или ошибочные)
* 
* Результаты сохраняются в ассоциативные контейнеры: mapMainDesc, mapSecDesc, mapValidChains.
* Также формируется карта связей для построения цепочек видеофрагментов.
**/
void FileSystem_WFS::analysisIndexArea() {
	std::cout << "---------------------------------------------------------------------" << std::endl;
	std::cout << "IndexArea analysis" << std::endl;
	std::cout << "---------------------------------------------------------------------" << std::endl;

	uint64_t ui64OffsetIndexArea = stWFSAllValue.ui64IndexAreaOffset;	// Смещение на расположение IndexArea
	uint32_t ui32SizeIndexArea;											// Размер IndexArea
	uint64_t ui64SizeIndexArea;											// Размер IndexArea
	ui64SizeIndexArea = stWFSAllValue.ui64IndexAreaOffsetEnd - stWFSAllValue.ui64IndexAreaOffset;
	if (ui64SizeIndexArea > 0xFFFFFFFF) {
		/*
		* Очень большой размер IndexArea, надо читать по частям и это тоже неправильно, так как размер
		* может быть очень большой - 4 GB
		*/
		throw std::runtime_error("FileSystem_WFS::analysisIndexArea() - IndexArea size too large (> 4GB)");
	}
	else {
		ui32SizeIndexArea = static_cast<uint32_t>(ui64SizeIndexArea);
	}
	std::unique_ptr<uint8_t[]> pUi8ReadData = readRawData(ui64OffsetIndexArea, ui32SizeIndexArea);

	void* vPointerCurPos;										// Текущая позиция области памяти с которой осуществляется работа
	uint32_t ui32SizeDescriptor = sizeof(WFSIndexAreaSecDesc);	// Размер дескриптора видеофрагмента

	for (uint32_t ui32MainCycleIteration = 0; ui32MainCycleIteration < stWFSAllValue.ui32CountAllVideoFragments; ui32MainCycleIteration++) {
		/*
		* Работа с главными дескрипторами описания видеофрагментов
		* 
		* Какой-то особенный видеофрагмент
		* Позиция IndexArea + Размер зарезервированных + 0x35D4
		* if (vPointerCurPos == reinterpret_cast<void*>(reinterpret_cast<uint64_t>(pUi8ReadData.get()) + (ui32SizeDescriptorVideoFragment * 0x35D4)))
		* 0x17600 + 0x800 + 0x35D4 = 0x1B3D4
		* Данный видеофрагмент находится в SuperBlock копии которая располагается по смещению 0x83000
		* Также установлено что по смещению 0x83000 располагается SuperBlock,
		* некоторые данные в нем отличаются. Нахождение данного блока вызвано какой-то ошибкой
		*/
		uint64_t ui64OffsetIterIndexVideoDesc = stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(ui32MainCycleIteration) * static_cast<uint64_t>(ui32SizeDescriptor);
		vPointerCurPos = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(pUi8ReadData.get()) + ui32MainCycleIteration * ui32SizeDescriptor);
		uint8_t ui8TypeDescriptor = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(vPointerCurPos) + 1);
		if (ui8TypeDescriptor == 0x02 || ui8TypeDescriptor == 0x03) {
			if (isLikelyMainDesc(ui32MainCycleIteration, ui32SizeDescriptor, vPointerCurPos)) {
				stWFSAllValue.ui32CountMainDesc++;

				// Работаем с данными в областью памяти (vPointerCurPos) как со структурой WFSIndexAreaMainDesc
				WFSIndexAreaMainDesc* stWFSIndexAreaMainDesc = reinterpret_cast<WFSIndexAreaMainDesc*>(vPointerCurPos);
				WFSMainDescAdvInfo stIndexAreaMainDesc;

				stIndexAreaMainDesc.ui16CountSecDesc = (uint32_t)(stWFSIndexAreaMainDesc->ui16CountSecDesc);
				stIndexAreaMainDesc.ui32IndexNextSecDesc = stWFSIndexAreaMainDesc->ui32IndexNextSecDesc;
				if (stIndexAreaMainDesc.ui32IndexNextSecDesc == 0xFFFFFFFF) {
					stIndexAreaMainDesc.ui64OffsetNextSecDesc = 0;
				}
				else {
					stIndexAreaMainDesc.ui64OffsetNextSecDesc = stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(stIndexAreaMainDesc.ui32IndexNextSecDesc) * static_cast<uint64_t>(ui32SizeDescriptor);
				}
				
				stIndexAreaMainDesc.ui32IndexCurrentMainDesc		= stWFSIndexAreaMainDesc->ui32IndexCurrentMainDesc;
				stIndexAreaMainDesc.ui64OffsetCurrentMainDesc		= ui64OffsetIterIndexVideoDesc;
				stIndexAreaMainDesc.stTimeStampStartVideoStream		= convertTime(stWFSIndexAreaMainDesc->ui32TimeStampStartVideoStream);
				stIndexAreaMainDesc.stTimeStampEndVideoStream		= convertTime(stWFSIndexAreaMainDesc->ui32TimeStampEndVideoStream);
				stIndexAreaMainDesc.ui16LastVideoFragmentSizeDBS	= stWFSIndexAreaMainDesc->ui16LastVideoFragmentSizeDBS;
				stIndexAreaMainDesc.ui8RecordOrderVideo				= stWFSIndexAreaMainDesc->ui8RecordOrderVideo;
				stIndexAreaMainDesc.ui8CameraNumber					= (stWFSIndexAreaMainDesc->ui8CameraNumber - 0x02) / 0x04 + 1;

				mapMainDesc[ui32MainCycleIteration] = stIndexAreaMainDesc;
				mapValidChains[ui32MainCycleIteration].pMainDes = &mapMainDesc[ui32MainCycleIteration];
			}
			else {
				std::cout << "FileSystem_WFS::analysisIndexArea() - Current data not MainDescriptor video fragment" << std::endl;
				dumpHex(vPointerCurPos, ui32SizeDescriptor, ui64OffsetIterIndexVideoDesc);
			}
			continue;
		}
		if (ui8TypeDescriptor == 0x01) {
			if (isLikelySecDesc(ui32SizeDescriptor, vPointerCurPos)) {
				stWFSAllValue.ui32CountSecDesc++;

				// Работаем с данными в областью памяти (vPointerCurPos) как со структурой structWFSIndexAreaSecondaryDescriptor
				WFSIndexAreaSecDesc* stWFSIndexAreaSecondaryDesc = reinterpret_cast<WFSIndexAreaSecDesc*>(vPointerCurPos);
				WFSSecDescAdvInfo stIndexAreaSecDesc;

				stIndexAreaSecDesc.ui16RelativeIndexCurSecDesc	= (uint32_t)(stWFSIndexAreaSecondaryDesc->ui16RelativeIndexCurSecDesc);
				stIndexAreaSecDesc.ui32IndexNextSecDesc			= stWFSIndexAreaSecondaryDesc->ui32IndexNextSecDesc;
				stIndexAreaSecDesc.ui64OffsetNextSecDesc		= stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(stIndexAreaSecDesc.ui32IndexNextSecDesc) * static_cast<uint64_t>(ui32SizeDescriptor);
				stIndexAreaSecDesc.ui32IndexCurrentSecDesc		= ui32MainCycleIteration;
				stIndexAreaSecDesc.ui64OffsetCurrentSecDesc		= stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(stIndexAreaSecDesc.ui32IndexCurrentSecDesc) * static_cast<uint64_t>(ui32SizeDescriptor);
				stIndexAreaSecDesc.ui32IndexPrevSecDesc			= stWFSIndexAreaSecondaryDesc->ui32IndexPrevSecDesc;
				stIndexAreaSecDesc.ui64OffsetPrevSecDesc		= stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(stIndexAreaSecDesc.ui32IndexPrevSecDesc) * static_cast<uint64_t>(ui32SizeDescriptor);
				stIndexAreaSecDesc.stTimeStampStartVideoSegment	= convertTime(stWFSIndexAreaSecondaryDesc->ui32TimeStampStartVideoFragment);
				stIndexAreaSecDesc.stTimeStampEndVideoSegment	= convertTime(stWFSIndexAreaSecondaryDesc->ui32TimeStampEndVideoFragment);
				stIndexAreaSecDesc.ui16LastVideoFragmentSizeDBS	= stWFSIndexAreaSecondaryDesc->ui16LastVideoFragmentSizeDBS;
				stIndexAreaSecDesc.ui32IndexMainDesc			= stWFSIndexAreaSecondaryDesc->ui32IndexMainDesc;
				stIndexAreaSecDesc.ui64OffsetMainDesc			= stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(stIndexAreaSecDesc.ui32IndexMainDesc) * static_cast<uint64_t>(ui32SizeDescriptor);
				stIndexAreaSecDesc.ui8RecordOrderVideo			= stWFSIndexAreaSecondaryDesc->ui8RecordOrderVideo;
				stIndexAreaSecDesc.ui8CameraNumber				= (stWFSIndexAreaSecondaryDesc->ui8CameraNumber - 0x02) / 0x04 + 1;

				mapSecDesc[ui32MainCycleIteration] = stIndexAreaSecDesc;
			}
			else {
				std::cout << "FileSystem_WFS::analysisIndexArea() - Current data not Secondary Descriptor video fragment" << std::endl;
				dumpHex(vPointerCurPos, ui32SizeDescriptor, ui64OffsetIterIndexVideoDesc);
			}
			continue;
		}
		if (ui8TypeDescriptor == 0xFE) {
			/*
				Вычисление адреса зарезервированного дескриптора видеофрагмента. Для работы как с числом
				значение указателя (адрес) необходимо преобразовать в соответствующий тип данных - reinterpret_cast<uint64_t>(pUi8ReadData.get())
			*/

			// Подсчет количества нулевых и не нулевых байт
			uint32_t ui32CountZeroValue = 0;
			uint32_t ui32CountNotZeroValue = 0;
			for (uint8_t ui8ChildrenCycleIteration = 0; ui8ChildrenCycleIteration < ui32SizeDescriptor; ui8ChildrenCycleIteration++) {
				uint8_t ui8ValueByte = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(vPointerCurPos) + ui8ChildrenCycleIteration);
				if (ui8ValueByte == 0)
					ui32CountZeroValue++;
				else
					ui32CountNotZeroValue++;
			}
			if ((ui32CountNotZeroValue == 1) and (ui32CountZeroValue == 31) and (ui32CountNotZeroValue + ui32CountZeroValue) == ui32SizeDescriptor) {
				stWFSAllValue.ui32CountReservedDesc++;
			}
			else {
				std::cout << "FileSystem_WFS::analysisIndexArea() - Current data not Reserved Descriptor video fragment" << std::endl;
				dumpHex(vPointerCurPos, ui32SizeDescriptor, ui64OffsetIterIndexVideoDesc);
			}
			continue;
		}
		else {
			std::cout << "FileSystem_WFS::analysisIndexArea() - Current data Another Descriptor video fragment" << std::endl;
			dumpHex(vPointerCurPos, ui32SizeDescriptor, ui64OffsetIterIndexVideoDesc);

			stWFSAllValue.ui32CountAnotherDesc++;
			continue;
		}
	}
	stWFSAllValue.ui32CountAllDesc = stWFSAllValue.ui32CountMainDesc + stWFSAllValue.ui32CountSecDesc + stWFSAllValue.ui32CountReservedDesc;
}

/**
* \brief
* Вывод участка данных в консоль.
*
* \param
* const void* vPointOffsetPrintData - указатель на начало данных выводимой области.
* 
* uint32_t ui32SizePrintData - размер выводимой области.
* 
* uint64_t ui64OffsetInFWS - относительный адрес выводимой области исследуемых данных.
**/
void FileSystem_WFS::dumpHex(const void* vPointOffsetPrintData, uint32_t ui32SizePrintData, uint64_t ui64OffsetInFWS) {
	for (uint8_t ui32IterRow = 0; ui32IterRow < (ui32SizePrintData + 15) / 16; ui32IterRow++) {
		std::cout << "\t0x" << std::hex << ui64OffsetInFWS + static_cast<uint64_t>(ui32IterRow) * 16 << ": ";
		for (uint8_t ui8Column = 0; ui8Column < 16; ui8Column++) {
			uint8_t ui8PrintByte = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(vPointOffsetPrintData) + static_cast<uint64_t>(ui32IterRow) * 16 + ui8Column);
			std::cout << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ui8PrintByte) << " ";
		}
		std::cout << std::dec << std::endl;
	}
}

/**
* \brief
* Наполнение ассоциативного массива mapValidChains информацией MainDesc
* и непрерывной цепочкой SecDesc
**/
void FileSystem_WFS::rebuildUnwrittenVideoChain() {
	uint32_t ui32SizeDescriptor = sizeof(WFSIndexAreaMainDesc);

	uint32_t ui32CountMainDescWithoutSecDesc = 0;
	uint32_t ui32CountAddSecDesc = 0;
	uint32_t ui32CountMainDesc = 0;

	for (auto iterFragChain = mapValidChains.begin(); iterFragChain != mapValidChains.end(); ++iterFragChain) {
		ui32CountMainDesc++;
		
		uint32_t ui32IndexCurrentMainDesc = iterFragChain->first;
		FragmentChain& videoChainCurMainDesc = iterFragChain->second;

		uint16_t ui16CountSecDesc = videoChainCurMainDesc.pMainDes->ui16CountSecDesc;
		uint32_t ui32IndexNextSecDesc = videoChainCurMainDesc.pMainDes->ui32IndexNextSecDesc;

		uint64_t ui64OffsetCurrentMainDesc = stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(ui32IndexCurrentMainDesc) * static_cast<uint64_t>(ui32SizeDescriptor);

		uint8_t ui8CameraNumber = videoChainCurMainDesc.pMainDes->ui8CameraNumber;
		bool isFirst = true;
		bool isConsistentCameraNumber = true;

		/*
		* Проверка 1
		* Возможно, что MainDesc отсутствуют вторичные дескрипторы
		*/
		if (ui16CountSecDesc == 0 and (ui32IndexNextSecDesc == 0 or ui32IndexNextSecDesc == 0xFFFFFFFF)) {
			videoChainCurMainDesc.pMainDes->bIsAdd = true;
			ui32CountMainDescWithoutSecDesc++;
			continue;
		}

		/*
		* Проверка 2
		* Возможно, что MainDesc ссылается на не существующий SecDesc
		* Например, у MainDesc номер которого ui32IndexCurrentMainDesc	= 98264
		* if (ui32MainCycleIteration  == 4294967295)
		* Надо разобраться с ui32IndexCurrentMainDesc и ui32IndexNextSecDesc нормально их переименовать!!!!
		*/
		if ((ui32IndexNextSecDesc > 0) and (ui32IndexNextSecDesc <= stWFSAllValue.ui32CountAllVideoFragments)) {
			/*
			* Возможно, что первого фрагмента SecDesc который указан в MainDesc не существует
			* Поэтому необходимо осуществить проверку по индексу
			*/
			auto iterMapSecDesc = mapSecDesc.find(ui32IndexNextSecDesc);
			if (iterMapSecDesc == mapSecDesc.end()) {
				std::cout << "Warning: Secondary descriptor not found for index " << ui32IndexNextSecDesc << std::endl;

				std::unique_ptr<uint8_t[]> pUi8ReadData = readRawData(ui64OffsetCurrentMainDesc, ui32SizeDescriptor);
				dumpHex(pUi8ReadData.get(), ui32SizeDescriptor, videoChainCurMainDesc.pMainDes->ui64OffsetCurrentMainDesc);
				continue;
			}

			uint32_t ui32IndexCurrentSecDesc = iterMapSecDesc->second.ui32IndexCurrentSecDesc;
			uint32_t ui32IndexPrevSecDesc = iterMapSecDesc->second.ui32IndexPrevSecDesc;


			if (ui32IndexCurrentSecDesc != ui32IndexNextSecDesc) {
				std::cout << "Warning: Descriptor mismatch at " << ui32IndexNextSecDesc << std::endl;

				std::unique_ptr<uint8_t[]> pUi8ReadData = readRawData(ui64OffsetCurrentMainDesc, ui32SizeDescriptor);
				dumpHex(pUi8ReadData.get(), ui32SizeDescriptor, videoChainCurMainDesc.pMainDes->ui64OffsetCurrentMainDesc);
				continue;
			}

			/*
			* Можно проверить чему равен ui32IndexPrevSecDesc у первого вторичного дескриптора
			* У первого SecDesc это будет номер MainDesc
			*/
			if (ui32IndexPrevSecDesc != ui32IndexCurrentMainDesc) {
				std::cout << "Warning: the first SecDesc does not reference the MainDesc" << std::endl;

				std::unique_ptr<uint8_t[]> pUi8ReadData = readRawData(ui64OffsetCurrentMainDesc, ui32SizeDescriptor);
				dumpHex(pUi8ReadData.get(), ui32SizeDescriptor, videoChainCurMainDesc.pMainDes->ui64OffsetCurrentMainDesc);
				continue;
			}

			// Проверка номера камеры
			if (ui8CameraNumber != iterMapSecDesc->second.ui8CameraNumber) {
				std::cout << "Inconsistent camera numbers for recovery MainDesc " << std::endl;
			}

			// Далее осуществляется добавление первого фрагмента, информация о котором находится в MainDesc
			iterMapSecDesc->second.bIsAdd = true;
			videoChainCurMainDesc.pSecDes[0] = &iterMapSecDesc->second;
			ui32CountAddSecDesc++;
		}
		else {
			std::cout << "In current MainDesc: " << ui32IndexCurrentMainDesc << std::endl;
			std::cout << "\tSec Des not correct: " << ui32IndexNextSecDesc << std::endl;

			std::unique_ptr<uint8_t[]> pUi8ReadData = readRawData(ui64OffsetCurrentMainDesc, ui32SizeDescriptor);
			dumpHex(pUi8ReadData.get(), ui32SizeDescriptor, iterFragChain->second.pMainDes->ui64OffsetCurrentMainDesc);
			continue;
		}

		/*
		* Цикл для создания цепочки фрагментов уже на основании информации из SecDesc те начиная со 2 SecDesc
		*/
		for (uint32_t ui32IterSecDesc = 2; ui32IterSecDesc <= ui16CountSecDesc; ui32IterSecDesc++) {
			/*
			* На основании предыдущего (уже добавленного в цепочку) SecDesc имеется значение 
			* ui32IndexNextSecDesc о следующем значение те о 2
			*/
			auto iterMapSecDesc = mapSecDesc.find(ui32IndexNextSecDesc);
			if (iterMapSecDesc == mapSecDesc.end()) {
				std::cout << "Warning: Secondary descriptor not found in Video Chain " << ui32IndexCurrentMainDesc << std::endl;
				std::cout << "\tCurrent item " << ui32IterSecDesc << " in chain " << ui16CountSecDesc << std::endl;
				std::cout << "\tIndex SecDesc: " << ui32IndexNextSecDesc << std::endl;
				break;
			}

			// Получение из уже добавленного SecDesc следующий SecDesc
			ui32IndexNextSecDesc = iterMapSecDesc->second.ui32IndexNextSecDesc;

			// Переход на следующий SecDesc
			iterMapSecDesc = mapSecDesc.find(ui32IndexNextSecDesc);
			if (iterMapSecDesc == mapSecDesc.end()) {
				std::cout << "Warning: Secondary descriptor not found in Video Chain " << ui32IndexCurrentMainDesc << std::endl;
				std::cout << "\tCurrent item " << ui32IterSecDesc << " in chain " << ui16CountSecDesc << std::endl;
				std::cout << "\tIndex SecDesc: " << ui32IndexNextSecDesc << std::endl;
				break;
			}
			uint32_t ui32IndexCurrentSecDesc = iterMapSecDesc->second.ui32IndexCurrentSecDesc;

			uint32_t ui32OrderNextSecDesc = iterMapSecDesc->second.ui16RelativeIndexCurSecDesc;
			if (ui32IterSecDesc != ui32OrderNextSecDesc) {
				uint32_t ui32ChainSecSize = videoChainCurMainDesc.pSecDes.size();
				auto lastIt = std::prev(videoChainCurMainDesc.pSecDes.end());
				uint32_t ui32NextSecDesc = lastIt->second->ui32IndexNextSecDesc;

				std::cout << "Erase data:" << std::endl;
				std::cout << "\tCurrent MainDesc: " << ui32IndexCurrentMainDesc << std::endl;
				std::cout << "\tAmount all SecDesc from MainDesc: " << ui16CountSecDesc << std::endl;
				std::cout << "\tCurrent SecDesc incorrect: " << ui32IterSecDesc << std::endl;
				std::cout << "\tCurrent SecDesc from prev Sec: " << ui32OrderNextSecDesc << std::endl;
				std::cout << "\tAmount SecDesc in chain: " << ui32ChainSecSize << std::endl;
				std::cout << "\tNext SecDesc: " << ui32NextSecDesc << std::endl;
				continue;
			}

			// Проверка номера камеры
			if (ui8CameraNumber != iterMapSecDesc->second.ui8CameraNumber) {
				std::cout << "Inconsistent camera numbers for recovery MainDesc " << std::endl;
			}
			iterMapSecDesc->second.bIsAdd = true;
			videoChainCurMainDesc.pSecDes[ui32IterSecDesc - 1] = &iterMapSecDesc->second;
			ui32CountAddSecDesc++;
		}
		videoChainCurMainDesc.pMainDes->bIsAdd = true;
	}
}

/**
* \brief
* Дополнение mapValidChains невключенноми раннее SecDesc и наполнение 
* mapIncompleteChains восстановленными MainDesc и оставшимися SecDesc
**/
void FileSystem_WFS::rebuildOverwrittenVideoChain() {
	uint32_t ui32SizeDescriptor = sizeof(WFSIndexAreaMainDesc);
	uint32_t ui32AmountNotAdd = 0;


	for (auto iterMapSecDesc = mapSecDesc.begin(); iterMapSecDesc != mapSecDesc.end(); ++iterMapSecDesc) {
		WFSSecDescAdvInfo& iterSecDesc = iterMapSecDesc->second;
		if (iterSecDesc.bIsAdd == false) {
			ui32AmountNotAdd++;

			uint32_t ui32IndexMainDesc = iterSecDesc.ui32IndexMainDesc;

			auto iterFragmentChain = mapValidChains.find(ui32IndexMainDesc);
			if (iterFragmentChain == mapValidChains.end()) {
				auto itChain = mapIncompleteChains.find(ui32IndexMainDesc);
				if (itChain == mapIncompleteChains.end() || itChain->second.pMainDes == nullptr) {
					WFSMainDescAdvInfo stIndexAreaMainDesc{};
					stIndexAreaMainDesc.ui16CountSecDesc++;
					stIndexAreaMainDesc.ui32IndexCurrentMainDesc = ui32IndexMainDesc;
					stIndexAreaMainDesc.ui64OffsetCurrentMainDesc = stWFSAllValue.ui64IndexAreaOffset + static_cast<uint64_t>(ui32IndexMainDesc) * static_cast<uint64_t>(ui32SizeDescriptor);
					stIndexAreaMainDesc.bIsAdd = true;

					mapMainDesc[ui32IndexMainDesc] = stIndexAreaMainDesc;
					mapIncompleteChains[ui32IndexMainDesc].pMainDes = &mapMainDesc[ui32IndexMainDesc];
				}
				else {
					mapMainDesc[ui32IndexMainDesc].ui16CountSecDesc++;
				}
				iterSecDesc.bIsAdd = true;
				iterSecDesc.bIsRecovered = true;
				mapIncompleteChains[ui32IndexMainDesc].pSecDes[iterSecDesc.ui16RelativeIndexCurSecDesc] = &iterMapSecDesc->second;
				continue;
			}
			else {
				iterSecDesc.bIsAdd = true;
				/*
				* Скорее всего видеофрагмент уже переписан поэтому вторичный дескриптор который добавлен в цепочку стоит пометить
				*/
				iterSecDesc.bIsRecovered = true;
				iterFragmentChain->second.pSecDes[iterSecDesc.ui16RelativeIndexCurSecDesc] = &iterMapSecDesc->second;
			}
		}
	}

	// Заполнение MainDesc в неполных цепочках
	for (auto iterIncomFragChain = mapIncompleteChains.begin(); iterIncomFragChain != mapIncompleteChains.end(); ++iterIncomFragChain) {
		auto chainSedDesc = iterIncomFragChain->second.pSecDes;
		uint8_t ui8CameraNumber = 0;
		bool isFirst = true;
		bool isConsistentCameraNumber = true;

		WFSDateTime stMinStartDate = { 9999, 99, 99, 99, 99, 99 };
		WFSDateTime stMaxEndDate = { 0, 0, 0, 0, 0, 0 };

		for (auto iterSecDesc = chainSedDesc.begin(); iterSecDesc != chainSedDesc.end(); ++iterSecDesc) {
			WFSSecDescAdvInfo* pSecDesc = iterSecDesc->second;
			if (pSecDesc == nullptr) {
				continue;
			}
			if (isFirst) {
				ui8CameraNumber = pSecDesc->ui8CameraNumber;
				isFirst = false;
			}
			else if (pSecDesc->ui8CameraNumber != ui8CameraNumber) {
				isConsistentCameraNumber = false;
				break;
			}
			if (pSecDesc->stTimeStampStartVideoSegment.CompareTo(stMinStartDate) < 0) {
				stMinStartDate = pSecDesc->stTimeStampStartVideoSegment;
			}
			if (pSecDesc->stTimeStampEndVideoSegment.CompareTo(stMaxEndDate) > 0) {
				stMaxEndDate = pSecDesc->stTimeStampEndVideoSegment;
			}
		}

		if (isConsistentCameraNumber && iterIncomFragChain->second.pMainDes!= nullptr) {
			iterIncomFragChain->second.pMainDes->ui8CameraNumber = ui8CameraNumber;
			iterIncomFragChain->second.pMainDes->stTimeStampStartVideoStream = stMinStartDate;
			iterIncomFragChain->second.pMainDes->stTimeStampEndVideoStream = stMaxEndDate;
		}
		else {
			std::cout << "Inconsistent camera numbers for recovery MainDesc " << std::endl;
		}
	}

	ui32AmountNotAdd = 0;
	for (auto iterMapSecDesc = mapSecDesc.begin(); iterMapSecDesc != mapSecDesc.end(); ++iterMapSecDesc) {
		WFSSecDescAdvInfo iterSecDesc = iterMapSecDesc->second;
		if (iterSecDesc.bIsAdd == false) {
			ui32AmountNotAdd++;
		}
	}

	if (ui32AmountNotAdd != 0) {
		std::cout << "Amount SecDesc wich not add: " << ui32AmountNotAdd << std::endl;
	}
}

/**
* \brief
* Проверяет, является ли дескриптор Main.
*
* \param
* uint32_t inUi32IndexDesc - индекс проверяемого дескриптора.
*
* uint32_t inUi32SizeDescVideoFragment - размер дескриптора.
* 
* const void* inPoitCurrentPosition - указатель на начало области проверяемого дескриптора.
*
* \return
* Возвращает true, если проверяемый дескриптор является Main, иначе — false.
**/
bool FileSystem_WFS::isLikelyMainDesc(uint32_t inUi32IndexDesc, uint32_t inUi32SizeDescVideoFragment, const void* inPoitCurrentPosition) {
	/*
	* Далее реализована некоторая логика по определению является обрабатываемые 32 байта
	* дескриптором видеофрагмента. Для этого предложена логика по подсчету нулевых элементов.
	* Для начала надо подсчитать возможное количество нулевых и не нулевых байт.

		Подсчет количества нулевых и не нулевых байт
													знач		100%		мах 0
		uint8_t		ui8Padding						00			0x00		1
		uint8_t		wfsTypeFragmentDescriptor;		03			-			0
		uint16_t	wfsMainAmountSecendaryDesc;		23 00		-			2
		uint32_t	wfsPosPreviousDescriptor;		00 00 00 00	0x00000000	4
		uint32_t	wfsPosNextDescriptor;			44 00 00 00	-			4
		uint32_t	wfsTimeStampStartVideoStream;	e8 c7 88 5c	-			1
		uint32_t	wfsTimeStampEndVideoStream;		00 ca 88 5c	-			1
		uint8_t		wfsSomeValue1[2];				00 00		0x0000		2
		uint16_t	wfsSizeLastVideoFragment;		b8 07		-			2
		uint32_t	wfsPosMainDescriptor;			40 00 00 00	-			3
		uint8_t		wfsSomeValue2[2];				01 00		0x0000		1
		uint8_t		wfsRecordOrderVideo;			00			-			1
		uint8_t		wfsCameraNumber;				06			-			0
																			22
		max zero - 22
		min not zero - 10
	*/
	uint32_t ui32CountZeroValue = 0;
	uint32_t ui32CountNotZeroValue = 0;
	for (uint32_t ui32ChildrenCycleIteration = 0; ui32ChildrenCycleIteration < inUi32SizeDescVideoFragment; ui32ChildrenCycleIteration++) {
		uint8_t ui8ValueByte;

		ui8ValueByte = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(inPoitCurrentPosition) + ui32ChildrenCycleIteration);
		if (ui8ValueByte == 0) {
			ui32CountZeroValue++;
		}
		else {
			ui32CountNotZeroValue++;
		}
	}
	if ((ui32CountNotZeroValue >= 10) and (ui32CountZeroValue <= 22) and (ui32CountNotZeroValue + ui32CountZeroValue) == inUi32SizeDescVideoFragment) {
		WFSIndexAreaMainDesc* stWFSIndexAreaMainDesc = (WFSIndexAreaMainDesc*)inPoitCurrentPosition;

		if (inUi32IndexDesc != stWFSIndexAreaMainDesc->ui32IndexCurrentMainDesc) {
			std::cout << "FileSystem_WFS::isLikelyMainDesc() - Current MainDescriptor video fragment not same ui32IndexCurrentMainDesc" << std::endl;
			std::cout << "\tCurrent MainDescriptor video fragment - " << inUi32IndexDesc << std::endl;
			std::cout << "\tMainDescriptor video fragment from struct - " << stWFSIndexAreaMainDesc->ui32IndexCurrentMainDesc << std::endl;
			return false;
		}

		WFSDateTime startVideoStream = convertTime(stWFSIndexAreaMainDesc->ui32TimeStampStartVideoStream);
		if (!isValidDateTime(startVideoStream)) {
			std::cout << "FileSystem_WFS::isLikelyMainDesc() - Current MainDescriptor video fragment not have correct time Start video stream" << std::endl;
			std::cout << "\tError date time - ";
			printWFSDateTime(startVideoStream);
			std::cout << std::endl;
			return false;
		}

		WFSDateTime endVideoStream = convertTime(stWFSIndexAreaMainDesc->ui32TimeStampEndVideoStream);
		if (!isValidDateTime(endVideoStream)) {
			std::cout << "FileSystem_WFS::isLikelyMainDesc() - Current MainDescriptor video fragment not have correct time End video stream" << std::endl;
			std::cout << "\tError date time - ";
			printWFSDateTime(endVideoStream);
			std::cout << std::endl;
			return false;
		}

		if (stWFSIndexAreaMainDesc->ui32IndexPrevSecDesc != 0) {
			std::cout << "FileSystem_WFS::isLikelyMainDesc() - Current MainDescriptor video fragment not have correct ui32IndexPrevSecDesc" << std::endl;
			return false;
		}
		return true;
	}
	return false;
}

/**
* \brief
* Проверяет, является ли дескриптор Desc.
*
* \param
* uint32_t inUi32SizeDescVideoFragment - размер дескриптора.
*
* const void* inPoitCurrentPosition - указатель на начало области проверяемого дескриптора.
*
* \return
* Возвращает true, если проверяемый дескриптор является Desc, иначе — false.
**/
bool FileSystem_WFS::isLikelySecDesc(uint32_t inUi32SizeDescVideoFragment, const void* inPoitCurrentPosition) {
	/*
	* Далее реализована некоторая логика по определению является обрабатываемые 32 байта
	* дескриптором видеофрагмента. Для этого предложена логика по подсчету нулевых элементов.
	* Для начала надо подсчитать возможное количество нулевых и не нулевых байт.
	*
													знач		100%		мах 0
		uint8_t		ui8Padding						00			0x00		1
		uint8_t		wfsTypeFragmentDescriptor;		01			-			0
		uint16_t	wfsOrderDescriptor;				0e 00		-			1
		uint32_t	wfsPosPreviousDescriptor;		0e 12 00 00	-			3
		uint32_t	wfsPosNextDescriptor;			12 12 00 00	-			3
		uint32_t	wfsTimeStampStartVideoSegment;	34 2d 89 5c	-			1
		uint32_t	wfsTimeStampEndVideoSegment;	44 2d 89 5c	-			1
		uint8_t		wfsSomeValue1[2];				00 00		0x0000		2
		uint16_t	wfsSizeLastVideoFragment;		00 00		0x0000		2
		uint32_t	wfsPosMainDescriptor;			ef 11 00 00	-			3
		uint8_t		wfsSomeValue2[2];				01 00		0x0000		1
		uint8_t		wfsRecordOrderVideo;			00			-			1
		uint8_t		wfsCameraNumber;				06			-			0
																			19

		max zero - 19
		min not zero - 13
	*/

	uint32_t ui32CountZeroValue = 0;
	uint32_t ui32CountNotZeroValue = 0;
	for (uint32_t ui8ChildrenCycleIteration = 0; ui8ChildrenCycleIteration < inUi32SizeDescVideoFragment; ui8ChildrenCycleIteration++) {
		uint8_t ui8ValueByte;

		ui8ValueByte = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint64_t>(inPoitCurrentPosition) + ui8ChildrenCycleIteration);
		if (ui8ValueByte == 0)
			ui32CountZeroValue++;
		else
			ui32CountNotZeroValue++;
	}
	if ((ui32CountNotZeroValue >= 13) and (ui32CountZeroValue <= 19) and (ui32CountNotZeroValue + ui32CountZeroValue) == inUi32SizeDescVideoFragment) {
		WFSIndexAreaSecDesc* stWFSIndexAreaSecDesc = (WFSIndexAreaSecDesc*)inPoitCurrentPosition;
		WFSDateTime startVideoFragment = convertTime(stWFSIndexAreaSecDesc->ui32TimeStampStartVideoFragment);

		if (!isValidDateTime(startVideoFragment)) {
			std::cout << "FileSystem_WFS::isLikelySecDesc() - Current Secondary Descriptor video fragment not have correct time" << std::endl;
			std::cout << "\tError date time - ";
			printWFSDateTime(startVideoFragment);
			std::cout << std::endl;
			return false;
		}

		WFSDateTime endVideoFragment = convertTime(stWFSIndexAreaSecDesc->ui32TimeStampEndVideoFragment);
		if (!isValidDateTime(endVideoFragment)) {
			std::cout << "FileSystem_WFS::isLikelySecDesc() - Current Secondary Descriptor video fragment not have correct time" << std::endl;
			std::cout << "\tError date time - ";
			printWFSDateTime(endVideoFragment);
			std::cout << std::endl;
			return false;
		}
		return true;
	}
	return false;
}

/**
* \brief
* Функция для представления даты и времени из формата хранения WFS
* в структуру данных
*
* \param
* uint32_t inDwTimeValue - значение времени в формате WFS
* 
* \return
* WFSDateTime - структура, хранящая информацию о дате и времени
**/
WFSDateTime FileSystem_WFS::convertTime(uint32_t inUi32TimeValue) {
	/*
		Временные метки файловой системы WFS хранятся в обратном порядке (little-endian).
		Так значение 0xe8c7885c в памяти стоит рассматривать как 0x5C88C7E8
		Исходя из описания формата хранения в следующей последовательности хранятся
		год		- 6 бит
		месяц	- 4 бит
		день	- 5 бит
		часы	- 5 бит
		минуты	- 6 бит
		секунды	- 6 бит

		Пример представления числа 0x5C88C7E8:
		ГГГГГГММММДДДДДЧЧЧЧЧммммммсссссс
		01011100100010001100011111101000

		Получается:
		010111	23	Год
		0010	2	Месяц
		00100	4	День
		01100	12	Час
		011111	31	Минута
		101000	40	Секунда
		04.02.2023 12:31:40

		Работа алгоритма заключается в том что изначальное
		число сдвигается на нужное количество бит и потом
		вычисляется логическая операция "&" по битовой маске
		интересующего значения
	*/
	uint8_t ui8ShiftYear = 26;
	uint8_t ui8ShiftMonth = 22;
	uint8_t ui8ShiftDay = 17;
	uint8_t ui8ShiftHour = 12;
	uint8_t ui8ShiftMinuts = 6;
	uint8_t ui8ShiftSeconds = 0;

	uint8_t ui8MaskYear = 63;		// mask - 111111 6 единиц
	uint8_t ui8MaskMonth = 15;		// mask - 1111 4 единиц
	uint8_t ui8MaskDay = 31;		// mask - 11111 5 единиц
	uint8_t ui8MaskHour = 31;		// mask - 11111 5 единиц
	uint8_t ui8MaskMinuts = 63;		// mask - 111111 6 единиц
	uint8_t ui8MaskSeconds = 63;	// mask - 111111 6 единиц

	WFSDateTime stDataWFS;

	stDataWFS.ui16Year = inUi32TimeValue >> ui8ShiftYear;
	stDataWFS.ui16Year = 2000 + (stDataWFS.ui16Year & ui8MaskYear);

	stDataWFS.ui8Month = inUi32TimeValue >> ui8ShiftMonth;
	stDataWFS.ui8Month = stDataWFS.ui8Month & ui8MaskMonth;

	stDataWFS.ui8Day = inUi32TimeValue >> ui8ShiftDay;
	stDataWFS.ui8Day = stDataWFS.ui8Day & ui8MaskDay;

	stDataWFS.ui8Hour = inUi32TimeValue >> ui8ShiftHour;
	stDataWFS.ui8Hour = stDataWFS.ui8Hour & ui8MaskHour;

	stDataWFS.ui8Minute = inUi32TimeValue >> ui8ShiftMinuts;
	stDataWFS.ui8Minute = stDataWFS.ui8Minute & ui8MaskMinuts;

	stDataWFS.ui8Second = inUi32TimeValue >> ui8ShiftSeconds;
	stDataWFS.ui8Second = stDataWFS.ui8Second & ui8MaskSeconds;

	return stDataWFS;
}

/**
* \brief
* Отображает дату и время, содержащиеся в структуре WFSDateTime
*
* \param
* const WFSDateTime& inStDataWFS - Константная ссылка на структуру WFSDateTime с данными даты и времени.
*
* \return
* void
*
**/
void FileSystem_WFS::printWFSDateTime(const WFSDateTime& inStDataWFS) {
	printf("%02u.%02u.%04u %02u:%02u:%02u", inStDataWFS.ui8Day, inStDataWFS.ui8Month, inStDataWFS.ui16Year, inStDataWFS.ui8Hour, inStDataWFS.ui8Minute, inStDataWFS.ui8Second);
}

/**
* \brief
* Проверяет корректность значений даты и времени в структуре WFSDateTime
*
* Функция проверяет поля структуры даты и времени, что они находятся в допустимых диапазонах:
*
* - Год: от 2000 до 2099
*
* - Месяц: 1–12
*
* - День: 1–31
*
* - Часы: 0–23
*
* - Минуты и секунды: 0–59
*
* \param
* WFSDateTime& inStDateWFS - структура времени и даты проверяемая.
*
* \return
* bool - true если значения корректны, иначе false.
**/
bool FileSystem_WFS::isValidDateTime(const WFSDateTime& inStDateWFS) {
	if (inStDateWFS.ui16Year < 2000 || inStDateWFS.ui16Year > 2099)
		return false;
	if (inStDateWFS.ui8Month < 1 || inStDateWFS.ui8Month > 12)
		return false;
	if (inStDateWFS.ui8Day < 1 || inStDateWFS.ui8Day > 31)
		return false;
	if (inStDateWFS.ui8Hour > 23)
		return false;
	if (inStDateWFS.ui8Minute > 59)
		return false;
	if (inStDateWFS.ui8Second > 59)
		return false;

	return true;
}

void FileSystem_WFS::printValidChains(FragmentChain& inFragmentChain) {
	uint16_t ui16LocAmountSecDesc = inFragmentChain.pMainDes->ui16CountSecDesc;
	std::cout << "New Video Chain" << std::endl;
	std::cout << "[ ] - " << inFragmentChain.pMainDes->ui32IndexCurrentMainDesc << std::endl;
	//std::cout << " └────";
	for (uint16_t ui16Iter = 0; ui16Iter < ui16LocAmountSecDesc; ui16Iter++) {
		auto iterSecDesc = inFragmentChain.pSecDes.find(ui16Iter);
		if (iterSecDesc != inFragmentChain.pSecDes.end()) {
			std::cout << "\t[" << ui16Iter << "] - " << iterSecDesc->second->ui32IndexCurrentSecDesc << std::endl;
		}
		else {
			std::cout << "\t[" << ui16Iter << "] - X" << std::endl;
		}
	}
}

void FileSystem_WFS::printIncompleteChains(FragmentChain& inFragmentChain) {
	std::cout << "New Video Chain" << std::endl;
	std::cout << "[ ] - X"  << std::endl;
	//std::cout << " └────";
	for (uint16_t ui16Iter = 0; ui16Iter < 0xFFFF; ui16Iter++) {
		auto iterSecDesc = inFragmentChain.pSecDes.find(ui16Iter);
		if (iterSecDesc != inFragmentChain.pSecDes.end()) {
			std::cout << "\t[" << ui16Iter << "] - " << iterSecDesc->second->ui32IndexCurrentSecDesc << std::endl;
		}
	}
}

void FileSystem_WFS::printAllChains() {
	// Вывод Цепочек видео где MainDesc был распознан из Index Area
	for (auto iterFragChain = mapValidChains.begin(); iterFragChain != mapValidChains.end(); ++iterFragChain) {
		printValidChains(iterFragChain->second);
	}

	// Вывод Цепочек видео где MainDesc не был обнаружен в Index Area
	for (auto iterFragChain = mapIncompleteChains.begin(); iterFragChain != mapIncompleteChains.end(); ++iterFragChain) {
		printIncompleteChains(iterFragChain->second);
	}
}

/**
* \brief
* Сохранение цепочки видеофрагментов в один файл.
*
* \param
* const FragmentChain& inFragmentChain - структура с данными о расположении видеофрагментов.
* 
* const std::string& inString - полный путь к файлу.
**/
void FileSystem_WFS::saveVideoChain(const FragmentChain& inFragmentChain, const std::string& inString) {
	uint16_t ui16LocAmountSecDesc = inFragmentChain.pMainDes->ui16CountSecDesc;
	uint32_t ui32SizeVideoFragment = stWFSAllValue.ui32VideoFragmentSizeByte;
	uint64_t ui64OffsetCurrentFragment = stWFSAllValue.ui64DataAreaOffsetStart + static_cast<uint64_t>(inFragmentChain.pMainDes->ui32IndexCurrentMainDesc) * static_cast<uint64_t>(stWFSAllValue.ui32VideoFragmentSizeByte);

	std::unique_ptr<uint8_t[]> pUi8ReadData = readRawData(ui64OffsetCurrentFragment, ui32SizeVideoFragment);

	if (!inputFile_->writeToFileAppend(inString, pUi8ReadData, ui32SizeVideoFragment)) {
		throw std::runtime_error("FileSystem_WFS::saveVideoChain() - Failed to write MainDescriptor data");
	}

	for (uint16_t ui16Iter = 0; ui16Iter < ui16LocAmountSecDesc; ui16Iter++) {
		auto iterSecDesc = inFragmentChain.pSecDes.find(ui16Iter);
		if (iterSecDesc != inFragmentChain.pSecDes.end()) {
			if (ui16Iter == ui16LocAmountSecDesc - 1) {
				ui32SizeVideoFragment = iterSecDesc->second->ui16LastVideoFragmentSizeDBS * stWFSAllValue.ui32DiskBlockSize;
			}
			
			ui64OffsetCurrentFragment = stWFSAllValue.ui64DataAreaOffsetStart + static_cast<uint64_t>(iterSecDesc->second->ui32IndexCurrentSecDesc) * static_cast<uint64_t>(stWFSAllValue.ui32VideoFragmentSizeByte);

			pUi8ReadData = readRawData(ui64OffsetCurrentFragment, ui32SizeVideoFragment);

			if (!inputFile_->writeToFileAppend(inString, pUi8ReadData, ui32SizeVideoFragment)) {
				throw std::runtime_error("FileSystem_WFS::saveVideoChain() - Failed to write secondary fragment index " + std::to_string(ui16Iter));
			}
		}
	}
}

/**
* \brief
* Функция предназначена для сохранения отдельного фрагмента
* видео из файловой системы WFS в указанный путь.
*
* \param
* const WFSSecDescAdvInfo& inSecDesc - Структура, описывающая конкретный видеофрагмент SecDesc (секцию), 
* который необходимо сохранить. Включает в себя индекс и размер последнего фрагмента в блоках диска.
*
* const std::string& inString - полный путь к файлу, в который должен быть сохранён извлечённый видеофрагмент.
**/
void FileSystem_WFS::saveSecFragmentVideo(const WFSSecDescAdvInfo& inSecDesc, const std::string& inString) {
	uint32_t ui32SizeVideoFragment;
	if (inSecDesc.ui16LastVideoFragmentSizeDBS == 0) {
		ui32SizeVideoFragment = stWFSAllValue.ui32VideoFragmentSizeByte;
	}
	else {
		ui32SizeVideoFragment = inSecDesc.ui16LastVideoFragmentSizeDBS * stWFSAllValue.ui32DiskBlockSize;
	}
	
	uint64_t ui64OffsetCurrentFragment = stWFSAllValue.ui64DataAreaOffsetStart + static_cast<uint64_t>(inSecDesc.ui32IndexCurrentSecDesc) * static_cast<uint64_t>(stWFSAllValue.ui32VideoFragmentSizeByte);

	std::unique_ptr<uint8_t[]> pUi8ReadData = readRawData(ui64OffsetCurrentFragment, ui32SizeVideoFragment);

	if (!inputFile_->writeToFile(inString, pUi8ReadData, ui32SizeVideoFragment)) {
		throw std::runtime_error("FileSystem_WFS::saveSecFragmentVideo() - Can't write to file");
	}
}