#include "StdInc.h"
#include "regioncontrollerdialog.h"

#include "mapcontroller.h"

#include "../lib/mapping/CMap.h"

#include <algorithm>
#include <array>
#include <set>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

VCMI_LIB_USING_NAMESPACE

namespace
{
const std::array<QString, 8> defaultSubregionColors = {
	QStringLiteral("#ffff00"),
	QStringLiteral("#ff00ff"),
	QStringLiteral("#00ffff"),
	QStringLiteral("#ff0000"),
	QStringLiteral("#00ff00"),
	QStringLiteral("#0080ff"),
	QStringLiteral("#ff8000"),
	QStringLiteral("#ffffff")
};

QIcon colorIcon(const QString & color)
{
	QPixmap pixmap(18, 18);
	pixmap.fill(Qt::transparent);
	QPainter painter(&pixmap);
	painter.fillRect(1, 1, 16, 16, QColor(color));
	painter.setPen(Qt::black);
	painter.drawRect(1, 1, 16, 16);
	return QIcon(pixmap);
}
}

RegionControllerDialog::RegionControllerDialog(MapController & controller, QWidget * parent)
	: QDialog(parent)
	, controller(controller)
{
	setWindowTitle(tr("Region Controller"));
	resize(750, 520);

	territoryCombo = new QComboBox(this);
	for(int i = 0; i < static_cast<int>(territoryRoleJsonMap().size()); ++i)
	{
		auto role = static_cast<TerritoryRole>(i);
		territoryCombo->addItem(QString::fromStdString(territoryRoleDisplayName(role)), i);
	}

	regionsList = new QListWidget(this);
	subregionsList = new QListWidget(this);
	addRegionButton = new QPushButton(QStringLiteral("+"), this);
	removeRegionButton = new QPushButton(QStringLiteral("-"), this);
	addSubregionButton = new QPushButton(QStringLiteral("+"), this);
	removeSubregionButton = new QPushButton(QStringLiteral("-"), this);

	auto * mainLayout = new QVBoxLayout(this);
	auto * territoriesLayout = new QVBoxLayout();
	territoriesLayout->addWidget(new QLabel(tr("Territories"), this));
	territoriesLayout->addWidget(territoryCombo);
	territoriesLayout->addStretch();

	auto * regionsHeader = new QHBoxLayout();
	regionsHeader->addWidget(new QLabel(tr("Regions"), this));
	regionsHeader->addWidget(addRegionButton);
	regionsHeader->addWidget(removeRegionButton);
	regionsHeader->addStretch();

	auto * regionsLayout = new QVBoxLayout();
	regionsLayout->addLayout(regionsHeader);
	regionsLayout->addWidget(regionsList);

	auto * subregionsHeader = new QHBoxLayout();
	subregionsHeader->addWidget(new QLabel(tr("SubRegions"), this));
	subregionsHeader->addWidget(addSubregionButton);
	subregionsHeader->addWidget(removeSubregionButton);
	subregionsHeader->addStretch();

	auto * subregionsLayout = new QVBoxLayout();
	subregionsLayout->addLayout(subregionsHeader);
	subregionsLayout->addWidget(subregionsList);

	auto * columns = new QHBoxLayout();
	columns->addLayout(territoriesLayout, 1);
	columns->addLayout(regionsLayout, 2);
	columns->addLayout(subregionsLayout, 2);
	mainLayout->addLayout(columns);

	connect(territoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &RegionControllerDialog::refreshRegions);
	connect(regionsList, &QListWidget::currentRowChanged, this, &RegionControllerDialog::refreshSubregions);
	connect(addRegionButton, &QPushButton::clicked, this, &RegionControllerDialog::addRegion);
	connect(removeRegionButton, &QPushButton::clicked, this, &RegionControllerDialog::removeRegion);
	connect(addSubregionButton, &QPushButton::clicked, this, &RegionControllerDialog::addSubregion);
	connect(removeSubregionButton, &QPushButton::clicked, this, &RegionControllerDialog::removeSubregion);

	refreshRegions();
}

StrategicRegionMap * RegionControllerDialog::regionMap()
{
	if(!controller.map())
		return nullptr;
	return &controller.map()->strategicRegionMap;
}

TerritoryRole RegionControllerDialog::currentTerritory() const
{
	return static_cast<TerritoryRole>(territoryCombo->currentData().toInt());
}

Region * RegionControllerDialog::currentRegion()
{
	auto * map = regionMap();
	if(!map || !regionsList->currentItem())
		return nullptr;

	const int id = regionsList->currentItem()->data(Qt::UserRole).toInt();
	for(auto & region : map->regions)
		if(region.id == id)
			return &region;

	return nullptr;
}

void RegionControllerDialog::addRegion()
{
	auto * map = regionMap();
	if(!map)
		return;

	Region region;
	region.id = map->nextRegionId();
	region.name = "Region " + std::to_string(region.id);
	region.territoryRole = currentTerritory();
	map->regions.push_back(region);

	markChanged();
	refreshRegions();

	for(int i = 0; i < regionsList->count(); ++i)
	{
		if(regionsList->item(i)->data(Qt::UserRole).toInt() == region.id)
		{
			regionsList->setCurrentRow(i);
			break;
		}
	}
}

void RegionControllerDialog::removeRegion()
{
	auto * map = regionMap();
	auto * region = currentRegion();
	if(!map || !region)
		return;

	const int regionId = region->id;
	const auto regionName = QString::fromStdString(region->name);
	if(QMessageBox::question(this, tr("Remove region"), tr("Remove %1 and all its subregions?").arg(regionName)) != QMessageBox::Yes)
		return;

	std::set<int> removedSubregions(region->subregionIds.begin(), region->subregionIds.end());

	map->regions.erase(std::remove_if(map->regions.begin(), map->regions.end(), [regionId](const Region & candidate)
	{
		return candidate.id == regionId;
	}), map->regions.end());

	map->subregions.erase(std::remove_if(map->subregions.begin(), map->subregions.end(), [&removedSubregions](const Subregion & candidate)
	{
		return removedSubregions.count(candidate.id) > 0;
	}), map->subregions.end());

	map->connections.erase(std::remove_if(map->connections.begin(), map->connections.end(), [&removedSubregions](const RegionConnection & connection)
	{
		return removedSubregions.count(connection.fromSubregionId) > 0 || removedSubregions.count(connection.toSubregionId) > 0;
	}), map->connections.end());

	markChanged();
	refreshRegions();
}

void RegionControllerDialog::addSubregion()
{
	auto * map = regionMap();
	auto * region = currentRegion();
	if(!map || !region)
		return;

	Subregion subregion;
	subregion.id = map->nextSubregionId();
	subregion.name = "SubRegion " + std::to_string(subregion.id);
	subregion.regionId = region->id;
	subregion.color = defaultSubregionColors[(subregion.id - 1) % defaultSubregionColors.size()].toStdString();
	map->subregions.push_back(subregion);
	region->subregionIds.push_back(subregion.id);

	markChanged();
	refreshSubregions();
}

void RegionControllerDialog::removeSubregion()
{
	auto * map = regionMap();
	auto * region = currentRegion();
	if(!map || !region || !subregionsList->currentItem())
		return;

	const int subregionId = subregionsList->currentItem()->data(Qt::UserRole).toInt();

	for(auto & candidate : map->regions)
		candidate.subregionIds.erase(std::remove(candidate.subregionIds.begin(), candidate.subregionIds.end(), subregionId), candidate.subregionIds.end());

	map->subregions.erase(std::remove_if(map->subregions.begin(), map->subregions.end(), [subregionId](const Subregion & candidate)
	{
		return candidate.id == subregionId;
	}), map->subregions.end());

	map->connections.erase(std::remove_if(map->connections.begin(), map->connections.end(), [subregionId](const RegionConnection & connection)
	{
		return connection.fromSubregionId == subregionId || connection.toSubregionId == subregionId;
	}), map->connections.end());

	markChanged();
	refreshSubregions();
}

void RegionControllerDialog::refreshRegions()
{
	regionsList->clear();
	subregionsList->clear();

	auto * map = regionMap();
	if(!map)
		return;

	const auto role = currentTerritory();
	for(const auto & region : map->regions)
	{
		if(region.territoryRole != role)
			continue;

		auto * item = new QListWidgetItem(QString::fromStdString(region.name), regionsList);
		item->setData(Qt::UserRole, region.id);
	}

	if(regionsList->count() > 0)
		regionsList->setCurrentRow(0);
	else
		refreshSubregions();
}

void RegionControllerDialog::refreshSubregions()
{
	subregionsList->clear();
	auto * map = regionMap();
	auto * region = currentRegion();
	if(!map || !region)
		return;

	for(const auto subregionId : region->subregionIds)
	{
		auto iter = std::find_if(map->subregions.begin(), map->subregions.end(), [subregionId](const Subregion & subregion)
		{
			return subregion.id == subregionId;
		});

		if(iter == map->subregions.end())
			continue;

		auto * item = new QListWidgetItem(colorIcon(QString::fromStdString(iter->color)), QString::fromStdString(iter->name), subregionsList);
		item->setData(Qt::UserRole, iter->id);
	}
}

void RegionControllerDialog::markChanged()
{
	controller.commitChangeWithoutRedraw();
	Q_EMIT strategicRegionMapChanged();
}
