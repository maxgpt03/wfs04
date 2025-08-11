#include "utils.h"

QString formatTimestamp(const WFSDateTime& t) {
	return QString::asprintf("%02u:%02u:%02u %02u.%02u.%04u",
		t.ui8Hour, t.ui8Minute, t.ui8Second,
		t.ui8Day, t.ui8Month, t.ui16Year);
}

std::unique_ptr<IFile> createPlatformFile() {
#if defined(__MACH__) && defined(__APPLE__)
	return std::make_unique<macFile>();
#elif defined(_WIN32)
	return std::make_unique<WinFile>();
#else
	return nullptr;
#endif
}

QDateTime toQDateTime(const WFSDateTime& wfsTime) {
	QDate date(wfsTime.ui16Year, wfsTime.ui8Month, wfsTime.ui8Day);
	QTime time(wfsTime.ui8Hour, wfsTime.ui8Minute, wfsTime.ui8Second);
	return QDateTime(date, time);
}