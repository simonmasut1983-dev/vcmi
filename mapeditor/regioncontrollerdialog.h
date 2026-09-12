#pragma once

#include <QDialog>

#include "../lib/mapping/StrategicRegionMap.h"

class QListWidget;
class QComboBox;
class QPushButton;
class MapController;

class RegionControllerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit RegionControllerDialog(MapController & controller, QWidget * parent = nullptr);

signals:
	void strategicRegionMapChanged();

private slots:
	void addRegion();
	void removeRegion();
	void addSubregion();
	void removeSubregion();
	void keepMiddleTerritoryCreatures();
	void refreshRegions();
	void refreshSubregions();

private:
	MapController & controller;
	QComboBox * territoryCombo = nullptr;
	QListWidget * regionsList = nullptr;
	QListWidget * subregionsList = nullptr;
	QPushButton * addRegionButton = nullptr;
	QPushButton * removeRegionButton = nullptr;
	QPushButton * addSubregionButton = nullptr;
	QPushButton * removeSubregionButton = nullptr;
	QPushButton * keepMiddleCreaturesButton = nullptr;

	VCMI_LIB_WRAP_NAMESPACE(StrategicRegionMap) * regionMap();
	VCMI_LIB_WRAP_NAMESPACE(Region) * currentRegion();
	VCMI_LIB_WRAP_NAMESPACE(TerritoryRole) currentTerritory() const;
	void markChanged();
};
