/*
 * TurnOrderProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TurnOrderProcessor.h"
#include "HeroPoolProcessor.h"
#include "NewTurnProcessor.h"
#include "PlayerMessageProcessor.h"

#include "../queries/QueriesProcessor.h"
#include "../queries/MapQueries.h"
#include "../CGameHandler.h"
#include "../CVCMIServer.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/CGCreature.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/pathfinder/CPathfinder.h"
#include "../../lib/pathfinder/PathfinderOptions.h"

TurnOrderProcessor::TurnOrderProcessor(CGameHandler * owner):
	gameHandler(owner)
{

}

int TurnOrderProcessor::simturnsTurnsMaxLimit() const
{
	if (simturnsMaxDurationDays)
		return *simturnsMaxDurationDays;
	return gameHandler->gameInfo().getStartInfo()->simturnsInfo.optionalTurns;
}

int TurnOrderProcessor::simturnsTurnsMinLimit() const
{
	if (simturnsMinDurationDays)
		return *simturnsMinDurationDays;
	return gameHandler->gameInfo().getStartInfo()->simturnsInfo.requiredTurns;
}

std::vector<TurnOrderProcessor::PlayerPair> TurnOrderProcessor::computeContactStatus() const
{
	std::vector<PlayerPair> result;

	assert(actedPlayers.empty());
	assert(actingPlayers.empty());

	for (auto left : awaitingPlayers)
	{
		for(auto right : awaitingPlayers)
		{
			if (left == right)
				continue;

			if (computeCanActSimultaneously(left, right))
				result.push_back({left, right});
		}
	}
	return result;
}

void TurnOrderProcessor::updateAndNotifyContactStatus()
{
	auto newBlockedContacts = computeContactStatus();

	if (newBlockedContacts.empty())
	{
		// Simturns between all players have ended - send single global notification
		if (!blockedContacts.empty())
			gameHandler->playerMessages->broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.simturn.end"));
	}
	else
	{
		// Simturns between some players have ended - notify each pair
		for (auto const & contact : blockedContacts)
		{
			if (vstd::contains(newBlockedContacts, contact))
				continue;

			MetaString message;
			message.appendTextID("vcmi.broadcast.simturn.endBetween");
			message.replaceName(contact.a);
			message.replaceName(contact.b);

			gameHandler->playerMessages->broadcastSystemMessage(message);
		}
	}

	blockedContacts = newBlockedContacts;
}

bool TurnOrderProcessor::playersInContact(PlayerColor left, PlayerColor right) const
{
	// TODO: refactor, cleanup and optimize

	boost::multi_array<bool, 3> leftReachability;
	boost::multi_array<bool, 3> rightReachability;

	int3 mapSize = gameHandler->gameInfo().getMapSize();

	leftReachability.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);
	rightReachability.resize(boost::extents[mapSize.z][mapSize.x][mapSize.y]);

	const auto * leftInfo = gameHandler->gameInfo().getPlayerState(left, false);
	const auto * rightInfo = gameHandler->gameInfo().getPlayerState(right, false);

	for (auto obj : gameHandler->gameState().getMap().getObjects())
	{
		if (obj->asOwnable())
		{
			int3 pos = obj->visitablePos();
			if (obj->getOwner() == left)
				leftReachability[pos.z][pos.x][pos.y] = true;
			if (obj->getOwner() == right)
				rightReachability[pos.z][pos.x][pos.y] = true;
		}
	}

	for(const auto & hero : leftInfo->getHeroes())
	{
		CPathsInfo out(mapSize, hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(out, gameHandler->gameState(), hero);
		config->options.ignoreGuards = true;
		config->options.turnLimit = 1;
		CPathfinder pathfinder(gameHandler->gameState(), config);
		pathfinder.calculatePaths();

		for (int z = 0; z < mapSize.z; ++z)
			for (int y = 0; y < mapSize.y; ++y)
				for (int x = 0; x < mapSize.x; ++x)
					if (out.getNode({x,y,z})->reachable())
						leftReachability[z][x][y] = true;
	}

	for(const auto & hero : rightInfo->getHeroes())
	{
		CPathsInfo out(mapSize, hero);
		auto config = std::make_shared<SingleHeroPathfinderConfig>(out, gameHandler->gameState(), hero);
		config->options.ignoreGuards = true;
		config->options.turnLimit = 1;
		CPathfinder pathfinder(gameHandler->gameState(), config);
		pathfinder.calculatePaths();

		for (int z = 0; z < mapSize.z; ++z)
			for (int y = 0; y < mapSize.y; ++y)
				for (int x = 0; x < mapSize.x; ++x)
					if (out.getNode({x,y,z})->reachable())
						rightReachability[z][x][y] = true;
	}

	for (int z = 0; z < mapSize.z; ++z)
		for (int y = 0; y < mapSize.y; ++y)
			for (int x = 0; x < mapSize.x; ++x)
				if (leftReachability[z][x][y] && rightReachability[z][x][y])
					return true;

	return false;
}

bool TurnOrderProcessor::isContactAllowed(PlayerColor active, PlayerColor waiting) const
{
	assert(active != waiting);
	return !vstd::contains(blockedContacts, PlayerPair{active, waiting});
}

bool TurnOrderProcessor::computeCanActSimultaneously(PlayerColor active, PlayerColor waiting) const
{
	const auto * activeInfo = gameHandler->gameInfo().getPlayerState(active, false);
	const auto * waitingInfo = gameHandler->gameInfo().getPlayerState(waiting, false);

	assert(active != waiting);
	assert(activeInfo);
	assert(waitingInfo);

	if (activeInfo->human != waitingInfo->human)
	{
		// only one AI and one human can play simultaneously from single connection
		if (!gameHandler->gameInfo().getStartInfo()->simturnsInfo.allowHumanWithAI)
			return false;
	}
	else
	{
		// two AI or two humans in hotseat can't play at the same time
		if (gameHandler->hasBothPlayersAtSameConnection(active, waiting))
			return false;
	}

	if (gameHandler->gameInfo().getDate(Date::DAY) < simturnsTurnsMinLimit())
		return true;

	if (gameHandler->gameInfo().getDate(Date::DAY) > simturnsTurnsMaxLimit())
		return false;

	if (gameHandler->gameInfo().getStartInfo()->simturnsInfo.ignoreAlliedContacts && activeInfo->team == waitingInfo->team)
		return true;

	if (playersInContact(active, waiting))
		return false;

	return true;
}

bool TurnOrderProcessor::mustActBefore(PlayerColor left, PlayerColor right) const
{
	const auto * leftInfo = gameHandler->gameInfo().getPlayerState(left, false);
	const auto * rightInfo = gameHandler->gameInfo().getPlayerState(right, false);

	assert(left != right);
	assert(leftInfo && rightInfo);

	if (!leftInfo)
		return false;
	if (!rightInfo)
		return true;

	if (leftInfo->isHuman() && !rightInfo->isHuman())
		return true;

	if (!leftInfo->isHuman() && rightInfo->isHuman())
		return false;

	return false;
}

bool TurnOrderProcessor::canStartTurn(PlayerColor which) const
{
	for (auto player : awaitingPlayers)
	{
		if (player != which && mustActBefore(player, which))
			return false;
	}

	for (auto player : actingPlayers)
	{
		if (player != which && isContactAllowed(player, which))
			return false;
	}

	return true;
}

bool TurnOrderProcessor::weeklySimturnsEnabled() const
{
	return gameHandler->gameInfo().getStartInfo()->extraOptionsInfo.weeklySimturns;
}
PlayerColor TurnOrderProcessor::playerForTerritoryRole(TerritoryRole role) const
{
	switch(role)
	{
	case TerritoryRole::PLAYER1_REALM:
		return PlayerColor(0);
	case TerritoryRole::PLAYER2_REALM:
		return PlayerColor(1);
	default:
		return PlayerColor::NEUTRAL;
	}
}

void TurnOrderProcessor::rebuildObjectRegionTable()
{
	objectRegionIds.clear();
	regionTerritoryRoles.clear();

	const auto & strategicMap = gameHandler->gameState().getMap().strategicRegionMap;
	std::map<int, int> subregionToRegion;
	for(const auto & region : strategicMap.regions)
	{
		regionTerritoryRoles[region.id] = region.territoryRole;
		for(const auto subregionId : region.subregionIds)
			subregionToRegion[subregionId] = region.id;
	}

	std::map<int3, int> tileRegionIds;
	int paintedTiles = 0;
	for(const auto & subregion : strategicMap.subregions)
	{
		int regionId = subregion.regionId;
		if(regionId < 0)
		{
			auto regionFromList = subregionToRegion.find(subregion.id);
			if(regionFromList != subregionToRegion.end())
				regionId = regionFromList->second;
		}

		if(regionId < 0)
			continue;

		for(const auto & tile : subregion.tiles)
		{
			tileRegionIds[tile] = regionId;
			paintedTiles++;
		}
	}

	std::map<TerritoryRole, int> assignedByRole;
	int assignedObjects = 0;
	int unassignedObjects = 0;
	for(const auto * object : gameHandler->gameState().getMap().getObjects())
	{
		if(!object)
			continue;

		auto tile = tileRegionIds.find(object->visitablePos());
		if(tile == tileRegionIds.end())
		{
			unassignedObjects++;
			continue;
		}

		objectRegionIds[object->id] = tile->second;
		assignedObjects++;

		auto role = regionTerritoryRoles.find(tile->second);
		if(role != regionTerritoryRoles.end())
			assignedByRole[role->second]++;
	}

	logGlobal->info("Weekly simturns: built runtime object region table: %d objects assigned, %d unassigned, %d regions, %d subregions, %d painted tiles.", assignedObjects, unassignedObjects, static_cast<int>(regionTerritoryRoles.size()), static_cast<int>(strategicMap.subregions.size()), paintedTiles);
	logGlobal->info("Weekly simturns: object territory assignment: player1=%d player2=%d middle=%d neutral=%d water=%d.", assignedByRole[TerritoryRole::PLAYER1_REALM], assignedByRole[TerritoryRole::PLAYER2_REALM], assignedByRole[TerritoryRole::MIDDLE_TERRITORY], assignedByRole[TerritoryRole::NEUTRAL], assignedByRole[TerritoryRole::WATER]);
}
TurnOrderProcessor::WeeklySimturnsWeekInfo TurnOrderProcessor::getOrCreateWeeklySimturnsWeekInfo(int localDay)
{
	const int daysPerWeek = LIBRARY->engineSettings()->getInteger(EGameSettings::GENERAL_DAYS_PER_WEEK);
	const int daysPerMonth = LIBRARY->engineSettings()->getInteger(EGameSettings::GENERAL_WEEKS_PER_MONTH) * daysPerWeek;
	const int dayOfWeek = CGameState::getDate(localDay, Date::DAY_OF_WEEK);
	const int weekStartDay = localDay - dayOfWeek + 1;

	auto existingDecision = weeklySimturnsWeekDecisions.find(weekStartDay);
	if(existingDecision != weeklySimturnsWeekDecisions.end())
		return existingDecision->second;

	const bool newMonth = ((weekStartDay - 1) % daysPerMonth) == 0;
	auto [weekType, creatureId, additionalGrowth] = gameHandler->newTurnProcessor->pickWeekType(newMonth);

	WeeklySimturnsWeekInfo info;
	info.startDay = weekStartDay;
	info.weekType = weekType;
	info.creatureId = creatureId;
	info.additionalGrowth = additionalGrowth;

	weeklySimturnsWeekDecisions[weekStartDay] = info;
	logGlobal->info("Weekly simturns: created server special week decision for local day %d: type=%d creature=%d additionalGrowth=%d.", weekStartDay, static_cast<int>(weekType), creatureId.num, additionalGrowth);

	return info;
}
std::optional<int> TurnOrderProcessor::getRegionIdForObject(ObjectInstanceID objectId) const
{
	auto region = objectRegionIds.find(objectId);
	if(region == objectRegionIds.end())
		return std::nullopt;
	return region->second;
}

std::optional<TerritoryRole> TurnOrderProcessor::territoryRoleForObject(ObjectInstanceID objectId) const
{
	auto regionId = getRegionIdForObject(objectId);
	if(!regionId)
		return std::nullopt;

	auto role = regionTerritoryRoles.find(*regionId);
	if(role == regionTerritoryRoles.end())
		return std::nullopt;
	return role->second;
}

bool TurnOrderProcessor::objectUsesLocalClockForPlayer(ObjectInstanceID objectId, PlayerColor player) const
{
	auto role = territoryRoleForObject(objectId);
	if(!role)
		return false;

	return playerForTerritoryRole(*role) == player;
}

std::map<PlayerColor, int> TurnOrderProcessor::getWeeklySimturnsPlayerDaysForDisplay() const
{
	if(!weeklySimturnsEnabled())
		return {};

	if(gameHandler->gameInfo().getDate(Date::DAY) >= simturnsTurnsMinLimit())
		return {};

	return playerDays;
}
int TurnOrderProcessor::getLocalDateForObject(ObjectInstanceID objectId, Date mode) const
{
	const int globalDay = gameHandler->gameState().getDate(Date::DAY);
	if(!weeklySimturnsEnabled() || globalDay >= simturnsTurnsMinLimit())
		return gameHandler->gameState().getDate(mode);

	auto role = territoryRoleForObject(objectId);
	if(!role)
		return gameHandler->gameState().getDate(mode);

	PlayerColor player = playerForTerritoryRole(*role);
	if(!player.isValidPlayer())
		return gameHandler->gameState().getDate(mode);

	auto day = playerDays.find(player);
	if(day == playerDays.end())
		return gameHandler->gameState().getDate(mode);

	return CGameState::getDate(day->second, mode);
}

bool TurnOrderProcessor::allWeeklySimturnsPlayersAwaitPhaseEnd() const
{
	size_t activePlayers = 0;
	for(const auto & player : gameHandler->gameState().players)
	{
		if(player.first.isValidPlayer() && player.second.status == EPlayerStatus::INGAME)
			activePlayers++;
	}

	return activePlayers > 0 && awaitingWeeklySimturnsPhaseEndPlayers.size() == activePlayers;
}

bool TurnOrderProcessor::weeklySimturnsPhaseEndsBetween(int startDay, int endDay) const
{
	const int phaseEndDay = simturnsTurnsMinLimit();
	return phaseEndDay > 0 && startDay < phaseEndDay && endDay >= phaseEndDay;
}

void TurnOrderProcessor::removeWeeklySimturnsPhaseCreatures()
{
	std::vector<ObjectInstanceID> creatureIds;

	for(const auto * object : gameHandler->gameState().getMap().getObjects())
	{
		const auto * creature = dynamic_cast<const CGCreature *>(object);
		if(creature && creature->removeAfterSimturnsPhase)
			creatureIds.push_back(creature->id);
	}

	int removed = 0;
	for(const auto & creatureId : creatureIds)
	{
		const auto * creature = gameHandler->gameInfo().getObj(creatureId, false);
		if(creature && gameHandler->removeObject(creature, PlayerColor::NEUTRAL))
			removed++;
	}

	logGlobal->info("Weekly simturns: removed %d marked neutral creatures at protected phase end.", removed);
}

void TurnOrderProcessor::doStartNewDay()
{
	assert(awaitingPlayers.empty());
	assert(actingPlayers.empty());

	gameHandler->onNewTurn();

	bool activePlayer = false;
	for (auto player : actedPlayers)
	{
		if (gameHandler->gameInfo().getPlayerState(player)->status == EPlayerStatus::INGAME)
			activePlayer = true;
	}

	if(!activePlayer)
	{
		gameHandler->gameServer().setState(EServerState::SHUTDOWN);
		return;
	}

	std::swap(actedPlayers, awaitingPlayers);

	updateAndNotifyContactStatus();
	tryStartTurnsForPlayers();
}

void TurnOrderProcessor::doSynchronizeWeeklySimturnsPhaseEnd()
{
	assert(actingPlayers.empty());
	assert(allWeeklySimturnsPlayersAwaitPhaseEnd());

	const int currentDay = gameHandler->gameInfo().getDate(Date::DAY);
	int nextSynchronizedDay = currentDay;
	for(auto player : awaitingWeeklySimturnsPhaseEndPlayers)
	{
		auto localDay = playerDays.find(player);
		if(localDay != playerDays.end())
			nextSynchronizedDay = std::max(nextSynchronizedDay, localDay->second + 1);
	}
	logGlobal->info("Weekly simturns: all players reached desynchronized phase end. Advancing global day to %d.", nextSynchronizedDay);

	while(gameHandler->gameInfo().getDate(Date::DAY) + 1 < nextSynchronizedDay)
		gameHandler->onNewTurn();

	gameHandler->onNewTurn();

	if(weeklySimturnsPhaseEndsBetween(currentDay, gameHandler->gameInfo().getDate(Date::DAY)))
		removeWeeklySimturnsPhaseCreatures();

	auto playersToRestart = awaitingWeeklySimturnsPhaseEndPlayers;
	awaitingWeeklySimturnsPhaseEndPlayers.clear();

	for(auto player : playersToRestart)
	{
		if(gameHandler->gameInfo().getPlayerStatus(player) != EPlayerStatus::INGAME)
			continue;

		playerDays[player] = gameHandler->gameInfo().getDate(Date::DAY);
		actedPlayers.erase(player);
		awaitingPlayers.insert(player);
	}

	updateAndNotifyContactStatus();
	tryStartTurnsForPlayers();
}

void TurnOrderProcessor::doStartPlayerTurn(PlayerColor which, bool applyStartOfTurnEffects)
{
	assert(gameHandler->gameInfo().getPlayerState(which));
	assert(gameHandler->gameInfo().getPlayerState(which)->status == EPlayerStatus::INGAME);

	// Only if player is actually starting his turn (and not loading from save)
	if (applyStartOfTurnEffects && !actingPlayers.count(which))
		gameHandler->onPlayerTurnStarted(which);

	actingPlayers.insert(which);
	awaitingPlayers.erase(which);

	PlayerStartsTurn pst;
	pst.player = which;

	bool timersActive = gameHandler->gameInfo().getStartInfo()->turnTimerInfo.isEnabled();
	bool isHuman = gameHandler->gameInfo().getPlayerState(which)->isHuman();

	if(timersActive && isHuman)
	{
		auto turnQuery = std::make_shared<TimerPauseQuery>(gameHandler, which);
		gameHandler->queries->addQuery(turnQuery);
		pst.queryID = turnQuery->queryID;
	}

	gameHandler->sendAndApply(pst);
	assert(!actingPlayers.empty());
}

void TurnOrderProcessor::doRestartWeeklyPlayerTurn(PlayerColor which)
{
	assert(isPlayerMakingTurn(which));

	actingPlayers.erase(which);

	PlayerEndsTurn pet;
	pet.player = which;
	gameHandler->sendAndApply(pet);

	doStartPlayerTurn(which, false);
}

void TurnOrderProcessor::doWaitForWeeklySimturnsPhaseEnd(PlayerColor which)
{
	assert(isPlayerMakingTurn(which));

	actingPlayers.erase(which);
	awaitingWeeklySimturnsPhaseEndPlayers.insert(which);
	logGlobal->info("Weekly simturns: player %s waits at local day %d.", which, playerDays.at(which));

	PlayerEndsTurn pet;
	pet.player = which;
	gameHandler->sendAndApply(pet);

	if(allWeeklySimturnsPlayersAwaitPhaseEnd())
		doSynchronizeWeeklySimturnsPhaseEnd();
}

void TurnOrderProcessor::doEndPlayerTurn(PlayerColor which)
{
	assert(isPlayerMakingTurn(which));
	assert(gameHandler->gameInfo().getPlayerStatus(which) == EPlayerStatus::INGAME);

	actingPlayers.erase(which);
	actedPlayers.insert(which);

	PlayerEndsTurn pet;
	pet.player = which;
	gameHandler->sendAndApply(pet);

	resumeTurnOrder();

	assert(!actingPlayers.empty());
}

void TurnOrderProcessor::addPlayer(PlayerColor which)
{
	awaitingPlayers.insert(which);

	if(weeklySimturnsEnabled() && gameHandler->gameInfo().getDate(Date::DAY) < simturnsTurnsMinLimit())
		playerDays.try_emplace(which, gameHandler->gameInfo().getDate(Date::DAY));
}

void TurnOrderProcessor::removePlayer(PlayerColor which)
{
	awaitingPlayers.erase(which);
	actingPlayers.erase(which);
	actedPlayers.erase(which);
	playerDays.erase(which);
	awaitingWeeklySimturnsPhaseEndPlayers.erase(which);
}

void TurnOrderProcessor::resumeTurnOrder()
{
	if (!awaitingPlayers.empty())
		tryStartTurnsForPlayers();

	if (actingPlayers.empty())
		doStartNewDay();
}

bool TurnOrderProcessor::onPlayerEndsTurn(PlayerColor which)
{
	if (!isPlayerMakingTurn(which))
	{
		gameHandler->complain("Can not end turn for player that is not acting!");
		return false;
	}

	if(gameHandler->gameInfo().getPlayerStatus(which) != EPlayerStatus::INGAME)
	{
		gameHandler->complain("Can not end turn for player that is not in game!");
		return false;
	}

	if(gameHandler->queries->topQuery(which) != nullptr)
	{
		gameHandler->complain("Cannot end turn before resolving queries!");
		return false;
	}

	if(weeklySimturnsEnabled() && gameHandler->gameInfo().getDate(Date::DAY) < simturnsTurnsMinLimit())
	{
		auto & playerDay = playerDays[which];
		if(playerDay == 0)
			playerDay = gameHandler->gameInfo().getDate(Date::DAY);

		if(playerDay >= simturnsTurnsMinLimit())
			doWaitForWeeklySimturnsPhaseEnd(which);
		else
		{
			playerDay++;
			logGlobal->debug("Weekly simturns: player %s advances to local day %d.", which, playerDay);
			gameHandler->newTurnProcessor->onWeeklySimturnsLocalDay(which);
			doRestartWeeklyPlayerTurn(which);
		}

		return true;
	}

	gameHandler->onPlayerTurnEnded(which);

	// it is possible that player have lost - e.g. spent 7 days without town
	// in this case - don't call doEndPlayerTurn - turn transfer was already handled by resumeTurnOrder
	if(gameHandler->gameInfo().getPlayerStatus(which) == EPlayerStatus::INGAME)
		doEndPlayerTurn(which);

	return true;
}

void TurnOrderProcessor::onGameStarted()
{
	if(gameHandler->gameInfo().getMapHeader()->battleOnly)
	{
		auto towns = gameHandler->gameState().getMap().getObjects<CGTownInstance>();
		auto heroes = gameHandler->gameState().getMap().getObjects<CGHeroInstance>();
		for (auto hero : heroes)
			if (auto cmd = hero->getCommander())
				const_cast<CCommanderInstance*>(cmd)->setAlive(false); // TODO: support commanders in battle mode setup
		if(!towns.size() && heroes.size() == 2)
			gameHandler->startBattle(heroes.at(0), heroes.at(1));
		else
			towns.at(0)->onHeroVisit(*gameHandler, heroes.at(0));

		return;
	}

	if (actingPlayers.empty())
		blockedContacts = computeContactStatus();

	if(weeklySimturnsEnabled() && gameHandler->gameInfo().getDate(Date::DAY) < simturnsTurnsMinLimit())
	{
		rebuildObjectRegionTable();
		for(const auto & player : gameHandler->gameState().players)
		{
			if(!player.first.isValidPlayer() || player.second.status != EPlayerStatus::INGAME)
				continue;

			auto playerDay = playerDays.try_emplace(player.first, gameHandler->gameInfo().getDate(Date::DAY)).first;
			if(playerDay->second == 0)
				playerDay->second = gameHandler->gameInfo().getDate(Date::DAY);
		}
	}

	// this may be game load - send notification to players that they can act
	auto actingPlayersCopy = actingPlayers;
	for (auto player : actingPlayersCopy)
		doStartPlayerTurn(player);

	tryStartTurnsForPlayers();
}

void TurnOrderProcessor::tryStartTurnsForPlayers()
{
	auto awaitingPlayersCopy = awaitingPlayers;
	for (auto player : awaitingPlayersCopy)
	{
		if (canStartTurn(player))
			doStartPlayerTurn(player);
	}
}

bool TurnOrderProcessor::isPlayerAwaitsTurn(PlayerColor which) const
{
	return vstd::contains(awaitingPlayers, which);
}

bool TurnOrderProcessor::isPlayerMakingTurn(PlayerColor which) const
{
	return vstd::contains(actingPlayers, which);
}

bool TurnOrderProcessor::isPlayerAwaitsNewDay(PlayerColor which) const
{
	return vstd::contains(actedPlayers, which);
}

void TurnOrderProcessor::setMinSimturnsDuration(int days)
{
	simturnsMinDurationDays = gameHandler->gameInfo().getDate(Date::DAY) + days;
}

void TurnOrderProcessor::setMaxSimturnsDuration(int days)
{
	simturnsMaxDurationDays = gameHandler->gameInfo().getDate(Date::DAY) + days;
}
