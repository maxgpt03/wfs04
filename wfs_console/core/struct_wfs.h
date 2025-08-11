#pragma once
#include <string>
#include <iostream>
#include <unordered_map>
#pragma pack(push, 1)

typedef struct WFSDateTime {
	uint16_t	ui16Year;
	uint8_t		ui8Month;
	uint8_t		ui8Day;
	uint8_t		ui8Hour;
	uint8_t		ui8Minute;
	uint8_t		ui8Second;

	int CompareTo(const WFSDateTime& other) const {
		if (ui16Year != other.ui16Year)
			return (ui16Year < other.ui16Year) ? -1 : 1;
		if (ui8Month != other.ui8Month)
			return (ui8Month < other.ui8Month) ? -1 : 1;
		if (ui8Day != other.ui8Day)
			return (ui8Day < other.ui8Day) ? -1 : 1;
		if (ui8Hour != other.ui8Hour)
			return (ui8Hour < other.ui8Hour) ? -1 : 1;
		if (ui8Minute != other.ui8Minute)
			return (ui8Minute < other.ui8Minute) ? -1 : 1;
		if (ui8Second != other.ui8Second)
			return (ui8Second < other.ui8Second) ? -1 : 1;
		return 0;
	}
};
static_assert(sizeof(WFSDateTime) == 7, "WFSDateTime size mismatch");

typedef struct WFSHeader
{
	uint8_t ui8Signature[6];						//	1	0x00	1	Сигнатура файловой системы. Возможные значения (Big-endian) 0x574653302E34 (WFS0.4)
	uint8_t ui8Padding[504];						//	2	0x06	504	Заполнение нулями
	uint8_t ui8EndHeader[2];						//	3	0x1EF	2	Конец заголовка. Значение (Big-endian) 0x584D (XM)
};
static_assert(sizeof(WFSHeader) == 512, "WFSHeader size mismatch");

/*
* Структура, используемая для интерпретации данных располагающихся в
* SuperBlock и являющиеся вторичными дескрипторами видеофрагментов
*/
typedef struct WFSSuperBlock {
	uint8_t		ui8Padding[8];							//	1	0x00	8	Поле заполнено нулями.
	uint8_t		ui8Reserved1[4];						//	2	0x08	4	Зарезервированное поле. Были замечены следующие значения: 0x4C (76), 0x01.
	uint8_t		ui8Reserved2[4];						//	3	0x0C	4	Зарезервированное поле. Были замечены следующие значения: 0xAA (170), 0x00.
	uint32_t	ui32TimeStampLastInDataBlock;			//	4	0x10	4	Временная метка последнего видео записанного в DataArea.
	uint32_t	ui32TimeStampLastWrite;					//	5	0x14	4	Временная метка последнего видео записанного оборудованием.
	uint32_t	ui32IndexAreaVideoFragmentPosLastWrite;	//	6	0x18	4	Последняя позиция дескриптора фрагмента в IndexArea записанная оборудованием.
	uint32_t	ui32IndexAreaVideoFragmentPosReWrite;	//	7	0x1C	4	Позиция первого допустимого дескриптора фрагмента после дескрипторов фрагментов, которые будут перезаписаны
	uint32_t	ui32CountAllVideoFragments;				//	8	0x20	4	Количество всех видеофрагментов в количестве дисковых блоков
	uint32_t	ui32TimeStampFistWillReWrite;			//	9	0x24	4	Временная метка первого допустимого фрагмента после дескрипторов фрагментов, которые будут перезаписаны
	uint32_t	ui32TimeStampFistVideo;					//	10	0x28	4	Начальная временная метка первого фрагмента видео в DataArea
	uint32_t	ui32DiskBlockSize;						//	11	0x2C	4	Размер дискового блока
	uint32_t	ui32VideoFragmentSizeDBS;				//	12	0x30	4	Размер видеофрагмента в количестве дисковых блоков
	uint8_t		ui8Reserved3[4];						//	13	0x34	4	Зарезервированное поле.
	uint32_t	ui32ReservedVideoFragmentCount;			//	14	0x38	4	Количество зарезервированных видеофрагмента
	uint8_t		ui8Reserved4[4];						//	15	0x3C	4	Зарезервированное поле.
	uint8_t		ui8Reserved5[4];						//	16	0x40	4	Зарезервированное поле.
	uint32_t	ui32IndexAreaPosStart;					//	17	0x44	4	Начальная позиция IndexArea (в дисковых блоках)
	uint32_t	ui32DataAreaPosStart;					//	18	0x48	4	Начальная позиция DataArea (в дисковых блоках)
	uint8_t		ui8Reserved6[4];						//	19	0x4C	4	Зарезервированное поле.
	uint8_t		ui8Reserved7[248];						//	20	0x50	248	Зарезервированное поле.
	uint32_t	ui32SuperBlockSignatures;				//	21	0x148	4	Сигнатура конца суперблока.
};
static_assert(sizeof(WFSSuperBlock) == 332, "WFSSuperBlock size mismatch");

/*
* Структура, используемая для интерпретации данных располагающихся в
* IndexArea и являющиеся основными дескрипторами видеофрагментов
*/
typedef struct WFSIndexAreaMainDesc {
	uint8_t		ui8Padding;							//	1	0x00	1	Заполнение нулями
	uint8_t		ui8TypeFragmentDesc;				//	2	0x01	1	Тип дескриптора - основной. Допустимые значения 0x02 или 0x03
	uint16_t	ui16CountSecDesc;					//	3	0x02	2	Информация о количестве второстепенных/дочерних дескрипторов. 
	uint32_t	ui32IndexPrevSecDesc;				//	4	0x04	4	Позиция в IndexArea дескриптора, предшествующего текущему. В основном дескрипторе значение 0x00000000
	uint32_t	ui32IndexNextSecDesc;				//	5	0x08	4	Позиция в IndexArea следующего дочернего дескриптора. При наличие дочернего дескрипторов значение не нулевое
	uint32_t	ui32TimeStampStartVideoStream;		//	6	0x0C	4	Метки времени начала видео.
	uint32_t	ui32TimeStampEndVideoStream;		//	7	0x10	4	Метки времени окончания видео.
	uint16_t	ui16Reserved1;						//	8	0x14	2	Зарезервированное поле. Заполнение нулями.
	uint16_t	ui16LastVideoFragmentSizeDBS;		//	9	0x16	2	Размер последнего фрагмента в DBS (512 байт). Подобного рода информация содержится в основном дескрипторе и последнем вторичном дескрипторе, в других случаях 0x0000. !ДЛЯ проверки!
	uint32_t	ui32IndexCurrentMainDesc;			//	10	0x18	4	Позиция MainDesc в IndexArea.
	uint16_t	ui16Reserved2;						//	11	0x1С	2	Зарезервированное поле. Заполнение нулями.
	uint8_t		ui8RecordOrderVideo;				//	12	0x1E	1	Порядок, в котором было записано видео. Он существует только в главном дескрипторе
	uint8_t		ui8CameraNumber;					//	13	0x1F	1	Номер камеры. 0x02 - 1 камера, 0x06 - 2, 0x0A - 3 ... То есть, он начинается с 0x02 и увеличивает 0x04
};
static_assert(sizeof(WFSIndexAreaMainDesc) == 32, "WFSIndexAreaMainDesc size mismatch");

/*
* Структура, используемая для хранения данных об основных дескрипторах
* видеофрагментов в векторе
*/
typedef struct WFSMainDescAdvInfo {
	uint16_t		ui16CountSecDesc = 0;			//	1	Количество вторичных дескрипторов
	uint32_t		ui32IndexNextSecDesc;			//	2	Номер следующего вторичного дескриптора
	uint64_t		ui64OffsetNextSecDesc;			//	3	Вычисляемое поле. Смещение следующего дескриптора относительно начала файла
	uint32_t		ui32IndexCurrentMainDesc;		//	4	Номер текущего дескриптора. Может браться из значения wfsNumberMainDescriptor или по значению итератора цикла
	uint64_t		ui64OffsetCurrentMainDesc;		//	5	Вычисляемое поле. Смещение текущего дескриптора относительно начала файла
	WFSDateTime		stTimeStampStartVideoStream;	//	6	Вычисляемое поле. Структура, содержит значение времени начала видео потока
	WFSDateTime		stTimeStampEndVideoStream;		//	8	Вычисляемое поле. Структура, содержит значение времени конца видео потока
	uint16_t		ui16LastVideoFragmentSizeDBS;	//	9	Размер последнего видеофрагмента, измеряемый в количестве дисковых блоков
	uint8_t			ui8RecordOrderVideo;			//	10	Порядок видеозаписи?
	uint8_t			ui8CameraNumber;				//	11	Номер камеры
	bool			bIsAdd = false;					//	12	Данный дескриптор обработан
};

/*
* Структура, используемая для интерпретации данных располагающихся в
* IndexArea и являющиеся вторичными дескрипторами видеофрагментов
*/
typedef struct WFSIndexAreaSecDesc {
	uint8_t		ui8Padding;							//	1	0x00	1	Заполнение нулями
	uint8_t		ui8TypeFragmentDesc;				//	2	0x01	1	Тип дескриптора - вторичный, значение 0x01 (продолжение видеопотока).
	uint16_t	ui16RelativeIndexCurSecDesc;		//	3	0x02	2	Относительный порядок после основного дескриптора
	uint32_t	ui32IndexPrevSecDesc;				//	4	0x04	4	Позиция в IndexArea дескриптора, предшествующего текущему
	uint32_t	ui32IndexNextSecDesc;				//	5	0x08	4	Позиция в IndexArea следующего дескриптора данного видеопотока. В последнем дескрипторе будет значение 0x00000000
	uint32_t	ui32TimeStampStartVideoFragment;	//	6	0x0C	4	Для вторичных дескрипторов - начальная временная метка соответствующего видеофрагмента
	uint32_t	ui32TimeStampEndVideoFragment;		//	7	0x10	4	Для вторичных дескрипторов - временная метка конца соответствующего видеофрагмента
	uint16_t	ui16Reserved1;						//	8	0x14	2	Зарезервированное поле.
	uint16_t	ui16LastVideoFragmentSizeDBS;		//	9	0x16	2	Размер последнего фрагмента в DBS (512 байт). Подобного рода информация содержится в основном дескрипторе и последнем вторичном дескрипторе, в других случаях 0x0000. !ДЛЯ проверки!
	uint32_t	ui32IndexMainDesc;					//	10	0x18	4	Позиция MainDesc в IndexArea
	uint16_t	ui16Reserved2;						//	11	0x1С	2	Зарезервированное поле.
	uint8_t		ui8RecordOrderVideo;				//	12	0x1E	1	Порядок, в котором было записано видео. Для вторичных дескрипторов - 0x00
	uint8_t		ui8CameraNumber;					//	13	0x1F	1	Номер камеры. 0x02 - 1 камера, 0x06 - 2, 0x0A - 3 ... То есть, он начинается с 0x02 и увеличивает 0x04
};
static_assert(sizeof(WFSIndexAreaSecDesc) == 32, "WFSIndexAreaSecDesc size mismatch");

/*
* Структура, используемая для хранения данных о вторичных дескрипторах
* видеофрагментов в векторе
*/
typedef struct WFSSecDescAdvInfo {
	uint16_t		ui16RelativeIndexCurSecDesc;	//	1	Номер текущего вторичного дескриптора из IndexArea относительно MainDesc
	uint32_t		ui32IndexNextSecDesc;			//	2	Номер следующего вторичного дескриптора
	uint64_t		ui64OffsetNextSecDesc;			//	3	Вычисляемое поле. Смещение следующего дескриптора
	uint32_t		ui32IndexCurrentSecDesc;		//	4	Номер текущего вторичного дескриптора из на основание значения итератора цикла
	uint64_t		ui64OffsetCurrentSecDesc;		//	5	Вычисляемое поле. Смещение текущего дескриптора
	uint32_t		ui32IndexPrevSecDesc;			//	6	Номер предыдущего вторичного дескриптора
	uint64_t		ui64OffsetPrevSecDesc;			//	7	Вычисляемое поле. Смещение предыдущего вторичного дескриптора
	WFSDateTime		stTimeStampStartVideoSegment;	//	8	Вычисляемое поле. Структура, содержит значение времени начала видео потока
	WFSDateTime		stTimeStampEndVideoSegment;		//	9	Вычисляемое поле. Структура, содержит значение времени конца видео потока
	uint16_t		ui16LastVideoFragmentSizeDBS;	//	10	Размер последнего видеофрагмента измеряемый в количестве дисковых блоков
	uint32_t		ui32IndexMainDesc;				//	11	Позиция в IndexArea основного дескриптора видео
	uint64_t		ui64OffsetMainDesc;				//	12	Вычисляемое поле. Смещение текущего дескриптора
	uint8_t			ui8RecordOrderVideo;			//	13	Порядок видеозаписи?
	uint8_t			ui8CameraNumber;				//	14	Номер камеры
	bool			bIsAdd = false;					//	15	Данный дескриптор обработан
	bool			bIsRecovered = false;			//	16	Данный дескриптор не содержался в основной цепочке
};

/*
* Структура, представляющая одну цепочку видеофрагмента.
* WFSMainDescAdvInfo* pMainDes - указатель на структуру главного дескриптора текущего видеофрагмента
* std::unordered_map<uint8_t, WFSSecDescAdvInfo*> pSecDes - хеш-таблица указателей на вторичные дескрипторы, 
* где ключом является, порядковый номер вторичного дескриптора.
*/
struct FragmentChain {
	WFSMainDescAdvInfo* pMainDes = nullptr;
	std::unordered_map<uint16_t, WFSSecDescAdvInfo*> pSecDes;
};

/*
* Основная структура, содержащая информацию о WFS
*/
typedef struct WFSAllValue {
	uint32_t	ui32DiskBlockSize;								// Размер дискового блока

	uint32_t	ui32VideoFragmentSizeDBS;						// Размер видеофрагмента в количестве дисковых блоков
	uint32_t	ui32CountAllVideoFragments;						// Общее количество видеофрагментов
	uint32_t	ui32ReservedVideoFragmentCount;					// Количество зарезервированных видеофрагмента
	uint32_t	ui32VideoFragmentSizeByte;						// Переменные для вычисления умножения больших чисел таких как возможна потеря значений
	uint64_t	ui64TotalVideoFragmentSizeBytes;				// Переменные для вычисления умножения больших чисел таких как возможна потеря значений
	uint64_t	ui64ReservedVideoFragmentSizeBytes;				// Переменные для вычисления умножения больших чисел таких как возможна потеря значений
	uint64_t	ui64UsedVideoFragmentSizeBytes;					// Переменные для вычисления умножения больших чисел таких как возможна потеря значений

	WFSDateTime	stWFSTimeStampLastInDataBlock;					// Временная метка последнего записанного видеофрагмента в область данных
	WFSDateTime	stWFSTimeStampLastWrite;						// Временная метка последнего записанного видеофрагмента
	WFSDateTime	stWFSTimeStampFistLast;							// Временная метка первого видеофрагмента, который будет перезаписан
	WFSDateTime	stWFSTimeStampFistVideo;						// Временная метка первого видеофрагмента в области данных

	uint32_t	ui32IndexAreaPosStart;							// Начальная позиция IndexArea. В количестве дескрипторных блоков
	uint64_t	ui64IndexAreaOffset;							// Начальная позиция IndexArea. В байтах
	uint64_t	ui64IndexAreaOffsetFirstRecord;					//
	uint64_t	ui64IndexAreaOffsetLastRecord;					//
	uint64_t	ui64IndexAreaOffsetEnd;							//
	uint32_t	ui32IndexAreaVideoFragmentPosLastWrite;			// Позиция в IndexArea последнего дескриптора видеофрагмента, записанного оборудованием. В байтах
	uint64_t	ui64IndexAreaVideoFragmentPosLastWriteOffset;	// Позиция в IndexArea последнего дескриптора видеофрагмента, записанного оборудованием. В количестве дескрипторных блоков
	uint32_t	ui32IndexAreaVideoFragmentPosReWrite;			// Позиция в IndexArea первого допустимого дескриптора фрагмента после дескрипторов фрагментов, которые будут перезаписаны. В байтах
	uint64_t	ui64IndexAreaVideoFragmentPosReWriteOffset;		// Позиция в IndexArea первого допустимого дескриптора фрагмента после дескрипторов фрагментов, которые будут перезаписаны. В количестве дескрипторных блоков
	uint64_t	ui64IndexAreaDescrSizeAllByte;					// Переменные для вычисления умножения больших чисел таких как возможна потеря значений
	uint64_t	ui64IndexAreaDescSizeReservedByte;				// Переменные для вычисления умножения больших чисел таких как возможна потеря значений
	uint64_t	ui64IndexAreaDescSizeUsedByte;					// Переменные для вычисления умножения больших чисел таких как возможна потеря значений

	uint32_t	ui32DataAreaPosStart;							// Начальная позиция DataArea. В количестве дескрипторных блоков
	uint64_t	ui64DataAreaOffsetStart;						// Начальная позиция DataArea. В байтах
	uint64_t	ui64DataAreaOffsetFirstRecord;					//
	uint64_t	ui64DataAreaOffsetLastRecord;					//
	uint64_t	ui64DataAreaOffsetEnd;							//

	uint32_t	ui32CountMainDesc = 0;							// Количество основных дескрипторов
	uint32_t	ui32CountSecDesc = 0;							// Количество вторичных дескрипторов
	uint32_t	ui32CountReservedDesc = 0;						// Количество зарезервированных дескрипторов
	uint32_t	ui32CountAnotherDesc = 0;						// Количество других дескрипторов
	uint32_t	ui32CountAllDesc = 0;							// Количество всех дескрипторов
};
#pragma pack(pop)