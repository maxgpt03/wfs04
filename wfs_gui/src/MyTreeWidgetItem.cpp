#include "MyTreeWidgetItem.h"

bool MyTreeWidgetItem::operator<(const QTreeWidgetItem& other) const {
	int column = treeWidget() ? treeWidget()->sortColumn() : 0;

	switch (column) {
		// Столбцы с Index и ui8CameraNumber — сортируем как число
		case 0:
		case 3:
			return data(column, Qt::UserRole).toInt() < other.data(column, Qt::UserRole).toInt();
		
		// Столбцы с Index и ui8CameraNumber — сортируем как число
		case 1: // Дата начала
		case 2: // Дата конца
			//return QDateTime::fromString(thisText, "dd.MM.yyyy HH:mm:ss") < QDateTime::fromString(otherText, "dd.MM.yyyy HH:mm:ss");
			return data(column, Qt::UserRole).toDateTime() < other.data(column, Qt::UserRole).toDateTime();

		default: // Остальные — строковая сортировка
			return text(column) < other.text(column);
	}
};