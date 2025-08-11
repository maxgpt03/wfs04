#pragma once

#include <QTreeWidgetItem>
#include <QDateTime>

class MyTreeWidgetItem : public QTreeWidgetItem {
public:
	using QTreeWidgetItem::QTreeWidgetItem;

	bool operator<(const QTreeWidgetItem& other) const override;
};