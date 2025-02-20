//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_shareddefs.h"
#include "tf_melee_mob.h"
#include "tf_mob_spawner.h"

LINK_ENTITY_TO_CLASS( tf_mob_spawner, CMobSpawner );

BEGIN_DATADESC( CMobSpawner )
	DEFINE_KEYFIELD( m_nMaxActiveMobs, FIELD_INTEGER, "max_mobs" ),
	DEFINE_KEYFIELD( m_nMobType, FIELD_INTEGER, "mob_type" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxActiveMobs", InputSetMaxActiveMobs ),
END_DATADESC()

CMobSpawner::CMobSpawner()
{
	m_bEnabled = false;
	m_nMaxActiveMobs = 1;
	m_nMobType = 0;
	m_nSpawned = 0;
}


void CMobSpawner::Spawn()
{
	BaseClass::Spawn();

	SetNextThink( gpGlobals->curtime );
}


void CMobSpawner::Think()
{
	m_activeMeleeMobs.FindAndFastRemove( NULL );

	if ( m_bEnabled && ( ( m_bInfiniteMeleeMobs && m_activeMeleeMobs.Count() < m_nMaxActiveMobs ) || ( !m_bInfiniteMeleeMobs && m_nSpawned < m_nMaxActiveMobs ) ) )
	{
		int nMeleeMobTeam = TF_TEAM_HALLOWEEN;
		int nSpawnerTeam = GetTeamNumber();
		if ( nSpawnerTeam == TF_TEAM_BLUE || nSpawnerTeam == TF_TEAM_RED )
		{
			nMeleeMobTeam = nSpawnerTeam;
		}
		CTFMeleeMob *pMeleeMob = CTFMeleeMob::SpawnAtPos( GetAbsOrigin(), m_flMeleeMobLifeTime, nMeleeMobTeam, NULL, (CTFMeleeMob::MobType_t)m_nMobType );
		if ( pMeleeMob )
		{
			m_nSpawned++;
			m_activeMeleeMobs.AddToTail( pMeleeMob );
		}

		SetNextThink( gpGlobals->curtime + RandomFloat( 1.5f, 3.f ) );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.2f );
}


void CMobSpawner::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;

	SetNextThink( gpGlobals->curtime );
}


void CMobSpawner::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	m_nSpawned = 0;

	m_activeMeleeMobs.Purge();

	SetNextThink( gpGlobals->curtime );
}


void CMobSpawner::InputSetMaxActiveMobs( inputdata_t &inputdata )
{
	m_nMaxActiveMobs = inputdata.value.Int();
}
