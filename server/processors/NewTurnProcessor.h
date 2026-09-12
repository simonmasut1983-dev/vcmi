/*
 * NewTurnProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/constants/Enumerations.h"
#include "../../lib/gameState/RumorState.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGTownInstance;
class ResourceSet;
struct SetAvailableCreatures;
struct SetMovePoints;
struct SetMana;
struct InfoWindow;
struct NewTurn;
struct WeeklySimturnsLocalDay;
VCMI_LIB_NAMESPACE_END

class CGameHandler;

class NewTurnProcessor : boost::noncopyable
{
	CGameHandler * gameHandler;

	std::vector<SetMana> updateHeroesManaPoints(std::optional<PlayerColor> player = std::nullopt);
	std::vector<SetMovePoints> updateHeroesMovementPoints(std::optional<PlayerColor> player = std::nullopt);

	ResourceSet generatePlayerIncome(PlayerColor playerID, bool includeDailyIncome, bool includeWeeklyIncome);
	SetAvailableCreatures generateTownGrowth(const CGTownInstance * town, EWeekType weekType, CreatureID creatureWeek, bool firstDay, int additionalGrowth);
	RumorState pickNewRumor();
	InfoWindow createInfoWindow(EWeekType weekType, CreatureID creatureWeek, bool newMonth, int additionalGrowth);

	void processWeeklySimturnsLocalMapObjects(PlayerColor player);
	void processWeeklySimturnsLocalTownBuildings(PlayerColor player);
	void onWeeklySimturnsLocalNewWeek(PlayerColor player, int localDay, WeeklySimturnsLocalDay & pack);

	NewTurn generateNewTurnPack(bool suppressNewWeekNotification = false);
	void handleTimeEvents(PlayerColor player);
	void handleTownEvents(const CGTownInstance *town);

	void updateNeutralTownGarrison(const CGTownInstance * t, int currentWeek) const;

public:
	NewTurnProcessor(CGameHandler * gameHandler);

	std::tuple<EWeekType, CreatureID, int> pickWeekType(bool newMonth);

	void onNewTurn(bool suppressNewWeekNotification = false);
	void onWeeklySimturnsLocalDay(PlayerColor player);
	void onPlayerTurnStarted(PlayerColor color);
	void onPlayerTurnEnded(PlayerColor color);
};
