// tf_mob_generator.cpp
// Entity to spawn a collection of mobs

#include "cbase.h"

#include "tf_mob_generator.h"
#include "tf_gamerules.h"
#include "tier3/tier3.h"

extern ConVar tf_max_active_mobs;

//------------------------------------------------------------------------------

BEGIN_DATADESC( CTFMobGenerator )
	DEFINE_KEYFIELD( m_spawnCount,		FIELD_INTEGER,	"count" ),
	DEFINE_KEYFIELD( m_maxActiveCount,	FIELD_INTEGER,	"maxActive" ),
	DEFINE_KEYFIELD( m_spawnInterval,	FIELD_FLOAT,	"interval" ),
	DEFINE_KEYFIELD( m_bSpawnOnlyWhenTriggered,			FIELD_INTEGER,	"spawnOnlyWhenTriggered" ),
	DEFINE_KEYFIELD( m_mobType,			FIELD_INTEGER,	"mobType" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnBot", InputSpawnBot ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RemoveBots", InputRemoveBots ),

	DEFINE_OUTPUT( m_onSpawned, "OnSpawned" ),
	DEFINE_OUTPUT( m_onExpended, "OnExpended" ),
	DEFINE_OUTPUT( m_onBotKilled, "OnBotKilled" ),

	DEFINE_THINKFUNC( GeneratorThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( tf_mob_generator, CTFMobGenerator );


//------------------------------------------------------------------------------
CTFMobGenerator::CTFMobGenerator( void )
	: m_spawnCount(0)
	, m_maxActiveCount(0)
	, m_spawnInterval(0.0f)
	, m_bSpawnOnlyWhenTriggered(false)
	, m_mobType(MobType_t::MELEE_NORMAL)
	, m_spawnCountRemaining(0)
	, m_bExpended(false)
	, m_bEnabled(false)
{
	SetThink( NULL );
}

//------------------------------------------------------------------------------
void CTFMobGenerator::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;

	if ( m_bExpended )
	{
		return;
	}

	SetThink( &CTFMobGenerator::GeneratorThink );

	if ( m_spawnCountRemaining )
	{
		// already generating - don't restart count
		return;
	}
	SetNextThink( gpGlobals->curtime );
	m_spawnCountRemaining = m_spawnCount;
}

//------------------------------------------------------------------------------
void CTFMobGenerator::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;

	// just stop thinking
	SetThink( NULL );
}

//------------------------------------------------------------------------------
void CTFMobGenerator::InputSpawnBot( inputdata_t &inputdata )
{
	if ( m_bEnabled )
	{
		SpawnBot();
	}
}

//------------------------------------------------------------------------------
void CTFMobGenerator::InputRemoveBots( inputdata_t &inputdata )
{
	for( int i = m_spawnedBotVector.Count() - 1; i >= 0 ; i-- )
	{
		CTFMeleeMob *pBot = m_spawnedBotVector[i];
		if ( pBot )
		{
			UTIL_Remove(pBot);
		}

		m_spawnedBotVector.FastRemove(i);
	}
}

//------------------------------------------------------------------------------
void CTFMobGenerator::OnBotKilled( CTFMeleeMob *pBot )
{
	m_onBotKilled.FireOutput( pBot, this );
}

//------------------------------------------------------------------------------

void CTFMobGenerator::Activate()
{
	BaseClass::Activate();
}

//------------------------------------------------------------------------------
void CTFMobGenerator::GeneratorThink( void )
{
	// still waiting for the real game to start?
	gamerules_roundstate_t roundState = TFGameRules()->State_Get();
	if ( roundState >= GR_STATE_TEAM_WIN || roundState < GR_STATE_PREROUND ||  TFGameRules()->IsInWaitingForPlayers() )
	{
		SetNextThink( gpGlobals->curtime + 1.0f );
		return;
	}

	// create the bot finally...
	if ( !m_bSpawnOnlyWhenTriggered )
	{
		SpawnBot();
	}
}

//------------------------------------------------------------------------------
void CTFMobGenerator::SpawnBot( void )
{
	// Clear dead mobs
	for ( int i = m_spawnedBotVector.Count() - 1; i >= 0; i-- )
	{
		CHandle< CTFMeleeMob > hBot = m_spawnedBotVector[i];
		if ( hBot == NULL )
		{
			m_spawnedBotVector.FastRemove(i);
		}
	}

	// Spawned too many?
	if ( m_spawnedBotVector.Count() >= m_maxActiveCount )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	// Spawned too many globally?
	// Probaly don't want this check so old mobs in a different area
	// don't stop new ones from spawning near players.
	// Mob spawn code kills the oldest.
	/*
	if ( ITFMeleeMobAutoList::AutoList().Count() >= tf_max_active_mobs )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}
	*/

	Vector vForward;
	AngleVectors( GetAbsAngles(), &vForward );

	CTFMeleeMob *pMob = CTFMeleeMob::SpawnAtPos(
		GetAbsOrigin(),
		GetAbsOrigin() + vForward,
		NULL,
		m_mobType,
		0.0f,
		TF_TEAM_HALLOWEEN
	);

	if ( pMob )
	{
		m_spawnedBotVector.AddToTail( pMob );

		m_onSpawned.FireOutput( pMob, this );

		--m_spawnCountRemaining;
		if ( m_spawnCountRemaining )
		{
			SetNextThink( gpGlobals->curtime + m_spawnInterval );
		}
		else
		{
			SetThink( NULL );
			m_onExpended.FireOutput( this, this );
			m_bExpended = true;
		}
	}
}
