/*
 * TurnOrderProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"
#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/constants/Enumerations.h"
#include "../../lib/mapping/StrategicRegionMap.h"

class CGameHandler;

class TurnOrderProcessor : boost::noncopyable
{
public:
	struct WeeklySimturnsWeekInfo
	{
		int startDay = 0;
		EWeekType weekType = EWeekType::NORMAL;
		CreatureID creatureId = CreatureID::NONE;
		int additionalGrowth = 0;

		template<typename Handler>
		void serialize(Handler & h)
		{
			h & startDay;
			h & weekType;
			h & creatureId;
			h & additionalGrowth;
		}
	};

private:
	CGameHandler * gameHandler;

	struct PlayerPair
	{
		PlayerColor a;
		PlayerColor b;

		bool operator == (const PlayerPair & other) const
		{
			return (a == other.a && b == other.b) || (a == other.b && b == other.a);
		}

		template<typename Handler>
		void serialize(Handler & h)
		{
			h & a;
			h & b;
		}
	};

	std::vector<PlayerPair> blockedContacts;

	std::set<PlayerColor> awaitingPlayers;
	std::set<PlayerColor> actingPlayers;
	std::set<PlayerColor> actedPlayers;

	std::map<PlayerColor, int> playerDays;
	std::set<PlayerColor> awaitingWeeklySimturnsPhaseEndPlayers;

	std::map<ObjectInstanceID, int> objectRegionIds;
	std::map<int, TerritoryRole> regionTerritoryRoles;
	std::map<int, WeeklySimturnsWeekInfo> weeklySimturnsWeekDecisions;

	std::optional<int> simturnsMinDurationDays;
	std::optional<int> simturnsMaxDurationDays;

	/// Returns date on which simturns must end unconditionally
	int simturnsTurnsMaxLimit() const;

	/// Returns date until which simturns must play unconditionally
	int simturnsTurnsMinLimit() const;

	/// Returns true if players are close enough to each other for their heroes to meet on this turn
	bool playersInContact(PlayerColor left, PlayerColor right) const;

	/// Returns true if waiting player can act alongside with currently acting player
	bool computeCanActSimultaneously(PlayerColor active, PlayerColor waiting) const;

	/// Returns true if left player must act before right player
	bool mustActBefore(PlayerColor left, PlayerColor right) const;

	/// Returns true if player is ready to start turn
	bool canStartTurn(PlayerColor which) const;

	/// Starts turn for all players that can start turn
	void tryStartTurnsForPlayers();

	void updateAndNotifyContactStatus();

	std::vector<PlayerPair> computeContactStatus() const;

	bool weeklySimturnsEnabled() const;
	std::optional<TerritoryRole> territoryRoleForObject(ObjectInstanceID objectId) const;
	PlayerColor playerForTerritoryRole(TerritoryRole role) const;
	bool allWeeklySimturnsPlayersAwaitPhaseEnd() const;
	bool weeklySimturnsPhaseEndsBetween(int startDay, int endDay) const;
	void removeWeeklySimturnsPhaseCreatures();

	void doStartNewDay();
	void doSynchronizeWeeklySimturnsPhaseEnd();
	void doStartPlayerTurn(PlayerColor which, bool applyStartOfTurnEffects = true);
	void doEndPlayerTurn(PlayerColor which);
	void doRestartWeeklyPlayerTurn(PlayerColor which);
	void doWaitForWeeklySimturnsPhaseEnd(PlayerColor which);

	bool isPlayerAwaitsTurn(PlayerColor which) const;
	bool isPlayerAwaitsNewDay(PlayerColor which) const;

public:
	TurnOrderProcessor(CGameHandler * owner);

	bool isContactAllowed(PlayerColor left, PlayerColor right) const;
	bool isPlayerMakingTurn(PlayerColor which) const;

	void rebuildObjectRegionTable();
	std::optional<int> getRegionIdForObject(ObjectInstanceID objectId) const;
	bool objectUsesLocalClockForPlayer(ObjectInstanceID objectId, PlayerColor player) const;
	int getLocalDateForObject(ObjectInstanceID objectId, Date mode = Date::DAY) const;
	std::map<PlayerColor, int> getWeeklySimturnsPlayerDaysForDisplay() const;
	WeeklySimturnsWeekInfo getOrCreateWeeklySimturnsWeekInfo(int localDay);

	/// Add new player to handle (e.g. on game start)
	void addPlayer(PlayerColor which);

	/// NetPack call-in
	bool onPlayerEndsTurn(PlayerColor which);

	/// Ends player turn and removes this player from turn order
	void removePlayer(PlayerColor which);

	/// Start turns for next players if possible
	void resumeTurnOrder();

	/// Start game (or resume from save) and send PlayerStartsTurn pack to player(s)
	void onGameStarted();

	/// Permanently override duration of contactless simultaneous turns
	void setMinSimturnsDuration(int days);

	/// Permanently override duration of simultaneous turns with contact detection
	void setMaxSimturnsDuration(int days);

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & blockedContacts;
		h & awaitingPlayers;
		h & actingPlayers;
		h & actedPlayers;
		h & simturnsMinDurationDays;
		h & simturnsMaxDurationDays;

		if(h.hasFeature(Handler::Version::WEEKLY_SIMTURNS_TURN_ORDER))
		{
			h & playerDays;
			h & awaitingWeeklySimturnsPhaseEndPlayers;

			if(h.hasFeature(Handler::Version::WEEKLY_SIMTURNS_WEEK_DECISIONS))
				h & weeklySimturnsWeekDecisions;
			else
				weeklySimturnsWeekDecisions.clear();

			objectRegionIds.clear();
			regionTerritoryRoles.clear();
		}
		else
		{
			playerDays.clear();
			awaitingWeeklySimturnsPhaseEndPlayers.clear();
			weeklySimturnsWeekDecisions.clear();
		}
	}
};
