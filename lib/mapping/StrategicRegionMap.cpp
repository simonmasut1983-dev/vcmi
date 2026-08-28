/*
 * StrategicRegionMap.cpp, part of VCMI engine
 *
 * Prototype data model for map-authored strategic regions.
 */

#include "StdInc.h"
#include "StrategicRegionMap.h"

#include "../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

const std::vector<std::string> & territoryRoleJsonMap()
{
	static const std::vector<std::string> result = {
		"player1Realm",
		"player2Realm",
		"middleTerritory",
		"neutral",
		"water"
	};
	return result;
}

std::string territoryRoleDisplayName(TerritoryRole role)
{
	switch(role)
	{
	case TerritoryRole::PLAYER1_REALM:
		return "Player 1 Realm";
	case TerritoryRole::PLAYER2_REALM:
		return "Player 2 Realm";
	case TerritoryRole::MIDDLE_TERRITORY:
		return "Middle Territory";
	case TerritoryRole::NEUTRAL:
		return "Neutral";
	case TerritoryRole::WATER:
		return "Water";
	}
	return "Neutral";
}

void serializeTile(JsonSerializeFormat & handler, int3 & tile)
{
	handler.serializeInt("x", tile.x);
	handler.serializeInt("y", tile.y);
	handler.serializeInt("z", tile.z);
}

void Subregion::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("id", id);
	handler.serializeString("name", name);
	handler.serializeString("color", color);
	handler.serializeInt("regionId", regionId, -1);
	handler.serializeBool("waterLike", waterLike, false);
	handler.serializeInt("dominantTerrain", dominantTerrain, -1);

	auto tilesArray = handler.enterArray("tiles");
	tilesArray.serializeStruct<int3>(tiles, serializeTile);
}

void Region::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("id", id);
	handler.serializeString("name", name);
	handler.serializeEnum("territoryRole", territoryRole, TerritoryRole::NEUTRAL, territoryRoleJsonMap());

	auto subregionsArray = handler.enterArray("subregions");
	subregionsArray.serializeArray(subregionIds);
}

void RegionConnection::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("id", id);
	handler.serializeInt("fromSubregionId", fromSubregionId, -1);
	handler.serializeInt("toSubregionId", toSubregionId, -1);
	handler.serializeString("type", type);
	handler.serializeString("state", state);
	handler.serializeInt("cost", cost, 0);
}

void StrategicRegionMap::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("version", version, 1);
	handler.enterArray("regions").serializeStruct(regions);
	handler.enterArray("subregions").serializeStruct(subregions);
	handler.enterArray("connections").serializeStruct(connections);
}

template<typename Container>
int nextId(const Container & container)
{
	int result = 1;
	for(const auto & item : container)
		result = std::max(result, item.id + 1);
	return result;
}

int StrategicRegionMap::nextRegionId() const
{
	return nextId(regions);
}

int StrategicRegionMap::nextSubregionId() const
{
	return nextId(subregions);
}

int StrategicRegionMap::nextConnectionId() const
{
	return nextId(connections);
}

VCMI_LIB_NAMESPACE_END
