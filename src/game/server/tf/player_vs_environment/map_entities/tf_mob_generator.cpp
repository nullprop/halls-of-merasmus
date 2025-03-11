// tf_mob_generator.cpp
// Entity to spawn a collection of mobs

#include "cbase.h"

#include "tf_mob_generator.h"
#include "tf_gamerules.h"
#include "tier3/tier3.h"

extern ConVar tf_max_active_mobs;

void CreateMob( const CCommand &args )
{
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( 1 );
	
	if ( !pPlayer )
		return;

	Vector vForward;
	AngleVectors( pPlayer->EyeAngles(), &vForward );
	const Vector vStart = pPlayer->EyePosition();
	const Vector vEnd = vStart + vForward * 1000.0f;

	trace_t trace;
	UTIL_TraceLine( vStart, vEnd, MASK_SOLID, pPlayer, COLLISION_GROUP_NONE, &trace );

	MobType_t mobType = MobType_t( ( args.ArgC() > 1 ) ? atoi(args[1]) : 0 );

	if (mobType < MobType_t::FLYING_NORMAL)
	{
		CTFMeleeMob::SpawnAtPos( trace.endpos, vec3_origin, NULL, mobType, 0.0f, TF_TEAM_HALLOWEEN );
	}
	else
	{
		CTFFlyingMob::SpawnAtPos( trace.endpos, vec3_origin, NULL, mobType, 0.0f, TF_TEAM_HALLOWEEN );
	}
}

ConCommand cc_create_mob( "create_mob", CreateMob, 0, FCVAR_CHEAT );

//------------------------------------------------------------------------------

BEGIN_DATADESC( CTFMobGenerator )
	DEFINE_KEYFIELD( m_spawnCount,		FIELD_INTEGER,	"count" ),
	DEFINE_KEYFIELD( m_maxActiveCount,	FIELD_INTEGER,	"maxActive" ),
	DEFINE_KEYFIELD( m_spawnInterval,	FIELD_FLOAT,	"interval" ),
	DEFINE_KEYFIELD( m_mobType,			FIELD_INTEGER,	"mobType" ),
	DEFINE_KEYFIELD( m_bSpawnOnlyWhenTriggered, FIELD_INTEGER, "spawnOnlyWhenTriggered" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SpawnMob", InputSpawnMob ),
	DEFINE_INPUTFUNC( FIELD_VOID, "RemoveMobs", InputRemoveMobs ),

	DEFINE_OUTPUT( m_onMobSpawned, "OnMobSpawned" ),
	DEFINE_OUTPUT( m_onMobKilled, "OnMobKilled" ),
	DEFINE_OUTPUT( m_onExpended, "OnExpended" ),

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
void CTFMobGenerator::InputSpawnMob( inputdata_t &inputdata )
{
	if ( m_bEnabled )
	{
		SpawnMob();
	}
}

//------------------------------------------------------------------------------
void CTFMobGenerator::InputRemoveMobs( inputdata_t &inputdata )
{
	for( int i = m_spawnedMobVector.Count() - 1; i >= 0 ; i-- )
	{
		NextBotCombatCharacter *pMob = m_spawnedMobVector[i];
		if ( pMob )
		{
			UTIL_Remove(pMob);
		}

		m_spawnedMobVector.FastRemove(i);
	}
}

//------------------------------------------------------------------------------
void CTFMobGenerator::OnMobKilled( NextBotCombatCharacter *pMob )
{
	m_onMobKilled.FireOutput( pMob, this );
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
		SpawnMob();
	}
}

//------------------------------------------------------------------------------
void CTFMobGenerator::SpawnMob( void )
{
	// Clear dead mobs
	for ( int i = m_spawnedMobVector.Count() - 1; i >= 0; i-- )
	{
		CHandle< NextBotCombatCharacter > hMob = m_spawnedMobVector[i];
		if ( hMob == NULL )
		{
			m_spawnedMobVector.FastRemove(i);
		}
	}

	// Spawned too many?
	if ( m_spawnedMobVector.Count() >= m_maxActiveCount )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	Vector vForward;
	AngleVectors( GetAbsAngles(), &vForward );

	NextBotCombatCharacter *pMob = NULL;
	
	switch (m_mobType)
	{
		default:
		case MobType_t::MELEE_NORMAL:
		case MobType_t::MELEE_GIANT:
		{
			pMob = 	CTFMeleeMob::SpawnAtPos(
				GetAbsOrigin(),
				GetAbsOrigin() + vForward,
				NULL,
				m_mobType,
				0.0f,
				TF_TEAM_HALLOWEEN
			);
			break;
		}

		case MobType_t::FLYING_NORMAL:
		{
			pMob = 	CTFFlyingMob::SpawnAtPos(
				GetAbsOrigin(),
				GetAbsOrigin() + vForward,
				NULL,
				m_mobType,
				0.0f,
				TF_TEAM_HALLOWEEN
			);
			break;
		}
	}

	if ( pMob )
	{
		m_spawnedMobVector.AddToTail( pMob );

		m_onMobSpawned.FireOutput( pMob, this );

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
