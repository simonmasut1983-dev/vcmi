/*
 * StrategicRegionMap.h, part of VCMI engine
 *
 * Prototype data model for map-authored strategic regions.
 */

#pragma once

#include "../int3.h"
#include "../serializer/Serializeable.h"

VCMI_LIB_NAMESPACE_BEGIN

class JsonSerializeFormat;

enum class TerritoryRole : ui8
{
	PLAYER1_REALM,
	PLAYER2_REALM,
	MIDDLE_TERRITORY,
	NEUTRAL,
	WATER
};

struct DLL_LINKAGE Subregion
{
	int id = 0;
	std::string name;
	std::string color;
	int regionId = -1;
	bool waterLike = false;
	int dominantTerrain = -1;
	std::vector<int3> tiles;

	void serializeJson(JsonSerializeFormat & handler);
};

struct DLL_LINKAGE Region
{
	int id = 0;
	std::string name;
	TerritoryRole territoryRole = TerritoryRole::NEUTRAL;
	std::vector<int> subregionIds;

	void serializeJson(JsonSerializeFormat & handler);
};

struct DLL_LINKAGE RegionConnection
{
	int id = 0;
	int fromSubregionId = -1;
	int toSubregionId = -1;
	std::string type = "land";
	std::string state = "open";
	int cost = 0;

	void serializeJson(JsonSerializeFormat & handler);
};

struct DLL_LINKAGE StrategicRegionMap
{
	int version = 1;
	std::vector<Region> regions;
	std::vector<Subregion> subregions;
	std::vector<RegionConnection> connections;

	void serializeJson(JsonSerializeFormat & handler);

	int nextRegionId() const;
	int nextSubregionId() const;
	int nextConnectionId() const;
};

DLL_LINKAGE const std::vector<std::string> & territoryRoleJsonMap();
DLL_LINKAGE std::string territoryRoleDisplayName(TerritoryRole role);

VCMI_LIB_NAMESPACE_END
