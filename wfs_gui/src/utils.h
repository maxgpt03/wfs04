#pragma once

#include <QString>
#include <QDateTime>

#include "core/struct_wfs.h"
#include "io/IFile.h"
#include "io/macFile.h"
#include "io/WinFile.h"

QString formatTimestamp(const WFSDateTime& t);
std::unique_ptr<IFile> createPlatformFile();
QDateTime toQDateTime(const WFSDateTime& wfsTime);