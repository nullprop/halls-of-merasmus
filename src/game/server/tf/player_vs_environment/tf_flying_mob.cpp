//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_mob_drop.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "NextBot/Path/NextBotChasePath.h"
#include "particle_parse.h"

#include "tf_flying_mob.h"
#include "mob_behavior/flying_mob_spawn.h"

#define FLYING_MOB_MODEL "models/props_halloween/halloween_demoeye.mdl"

ConVar tf_max_active_flying_mobs( "tf_max_active_flying_mobs", "8", FCVAR_CHEAT );


//-----------------------------------------------------------------------------------------------------
// NPC FlyingMob versions of the players
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_flying_mob, CTFFlyingMob );

IMPLEMENT_SERVERCLASS_ST( CTFFlyingMob, DT_TFFlyingMob )
	//SendPropFloat( SENDINFO( m_flHeadScale ) ),
END_SEND_TABLE()

IMPLEMENT_AUTO_LIST( ITFFlyingMobAutoList );

BEGIN_DATADESC( CTFFlyingMob )
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
END_DATADESC()


//-----------------------------------------------------------------------------------------------------
CTFFlyingMob::CTFFlyingMob()
{
	m_intention = new CTFFlyingMobIntention( this );
	m_locomotor = new CTFFlyingMobLocomotion( this );
	m_body = new CTFFlyingMobBody( this );

	m_nType = MobType_t::FLYING_NORMAL;

	m_flAttackRange = 800.f;
	m_flAttackDamage = 30.f;

	m_bDeathOutputFired = false;
}


//-----------------------------------------------------------------------------------------------------
CTFFlyingMob::~CTFFlyingMob()
{
	if ( m_intention )
		delete m_intention;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_body )
		delete m_body;
}


void CTFFlyingMob::PrecacheFlyingMob()
{
	int nSkeletonModel = PrecacheModel( FLYING_MOB_MODEL );
	PrecacheGibsForModel( nSkeletonModel );

	PrecacheScriptSound( "Halloween.EyeballBossLaugh" );
	PrecacheScriptSound( "Halloween.EyeballBossBigLaugh" );
	PrecacheScriptSound( "Halloween.EyeballBossDie" );

	PrecacheScriptSound( "Halloween.MonoculusBossSpawn" );
	PrecacheScriptSound( "Halloween.MonoculusBossDeath" );

	PrecacheParticleSystem( "eyeboss_death" );
	PrecacheParticleSystem( "eyeboss_aura_angry" );
	PrecacheParticleSystem( "eyeboss_aura_grumpy" );
	PrecacheParticleSystem( "eyeboss_aura_calm" );
}


//-----------------------------------------------------------------------------------------------------
void CTFFlyingMob::Precache()
{
	BaseClass::Precache();

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheFlyingMob();

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}


//-----------------------------------------------------------------------------------------------------
void CTFFlyingMob::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	AddFlag( FL_NPC );

	QAngle qAngle = vec3_angle;
	qAngle[YAW] = RandomFloat( 0, 360 );
	SetAbsAngles( qAngle );

	// Spawn Pos
	GetBodyInterface()->StartActivity( ACT_TRANSITION );

	// make sure I'm on TF_TEAM_HALLOWEEN
	ChangeTeam( TF_TEAM_HALLOWEEN );

	Vector mins( -50, -50, -50 );
	Vector maxs( 50, 50, 50 );
	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &mins, &maxs );
	CollisionProp()->SetCollisionBounds( mins, maxs );

	int angryPoseParameter = LookupPoseParameter( "anger" );
	if ( angryPoseParameter >= 0 )
	{
		SetPoseParameter( angryPoseParameter, 1 );
	}

	EmitSound( "Halloween.MonoculusBossSpawn" );

	// force kill oldest skeletons in the level (except skeleton king) to keep the number of skeletons under the max active
	int nForceKill = ITFFlyingMobAutoList::AutoList().Count() - tf_max_active_flying_mobs.GetInt();
	for ( int i=0; i<ITFFlyingMobAutoList::AutoList().Count() && nForceKill > 0; ++i )
	{
		CTFFlyingMob *pFlyingMob = static_cast< CTFFlyingMob* >( ITFFlyingMobAutoList::AutoList()[i] );
		pFlyingMob->ForceSuicide();
		nForceKill--;
	}
	Assert( nForceKill <= 0 );
}


//-----------------------------------------------------------------------------------------------------
int CTFFlyingMob::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( info.GetAttacker() && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
		return 0;

	DispatchParticleEffect( "spell_skeleton_goop_green", info.GetDamagePosition(), GetAbsAngles() );

	return BaseClass::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------------------------------
void CTFFlyingMob::Event_Killed( const CTakeDamageInfo &info )
{
	EmitSound( "Halloween.MonoculusBossDeath" ); // "Halloween.EyeballBossDie"
	DispatchParticleEffect( "eyeboss_death", me->GetAbsOrigin(), me->GetAbsAngles() );

	const Vector spawnOrigin = WorldSpaceCenter();
	const QAngle spawnAngles = QAngle();

	int cashAmount;
	int ammoAmount;
	int healthAmount;
	const char* cashClass;
	const char* ammoClass;
	const char* healthClass;

	switch ( m_nType )
	{
		case MobType_t::FLYING_NORMAL:
		default:
		{
			cashAmount = 100;
			ammoAmount = 20;
			healthAmount = 100;
			cashClass = "item_mobdrop_cash_small";
			ammoClass = "item_mobdrop_ammo_small";
			healthClass = "item_mobdrop_health_small";
			break;
		}
	}

	// Spawn some cash
	CTFMobDrop *pCash = assert_cast<CTFMobDrop*>(CBaseEntity::CreateNoSpawn(
		cashClass,
		spawnOrigin,
		spawnAngles,
		this
	));

	if ( pCash )
	{
		pCash->SetAmount( cashAmount );

		Vector vecImpulse = RandomVector( -1, 1 );
		vecImpulse.z = RandomFloat( 3.0f, 15.0f );
		VectorNormalize( vecImpulse );
		Vector vecVelocity = vecImpulse * 250.0 * RandomFloat( 1.0f, 4.0f );

		DispatchSpawn( pCash );
		pCash->DropSingleInstance( vecVelocity, this, 0, 0 );
	}

	// Spawn some ammo
	CTFMobDrop *pAmmo = assert_cast<CTFMobDrop*>(CBaseEntity::CreateNoSpawn(
		ammoClass,
		spawnOrigin,
		spawnAngles,
		this
	));

	if ( pAmmo )
	{
		pAmmo->SetAmount( ammoAmount );

		Vector vecImpulse = RandomVector( -1, 1 );
		vecImpulse.z = RandomFloat( 3.0f, 15.0f );
		VectorNormalize( vecImpulse );
		Vector vecVelocity = vecImpulse * 250.0 * RandomFloat( 1.0f, 4.0f );

		DispatchSpawn( pAmmo );
		pAmmo->DropSingleInstance( vecVelocity, this, 0, 0 );
	}

	// Spawn some health
	CTFMobDrop *pHealth = assert_cast<CTFMobDrop*>(CBaseEntity::CreateNoSpawn(
		healthClass,
		spawnOrigin,
		spawnAngles,
		this
	));

	if ( pHealth )
	{
		pHealth->SetAmount( healthAmount );

		Vector vecImpulse = RandomVector( -1, 1 );
		vecImpulse.z = RandomFloat( 3.0f, 15.0f );
		VectorNormalize( vecImpulse );
		Vector vecVelocity = vecImpulse * 250.0 * RandomFloat( 1.0f, 4.0f );

		DispatchSpawn( pHealth );
		pHealth->DropSingleInstance( vecVelocity, this, 0, 0 );
	}

	if (info.GetAttacker() && info.GetAttacker()->IsPlayer())
	{
		CTFPlayer *pPlayerAttacker = ToTFPlayer(info.GetAttacker());
		if (pPlayerAttacker)
		{
			if (TFGameRules() && TFGameRules()->IsHalloweenScenario(CTFGameRules::HALLOWEEN_SCENARIO_HIGHTOWER))
			{
				pPlayerAttacker->AwardAchievement(ACHIEVEMENT_TF_HALLOWEEN_HELLTOWER_SKELETON_GRIND);

				IGameEvent *pEvent = gameeventmanager->CreateEvent("halloween_skeleton_killed");
				if (pEvent)
				{
					pEvent->SetInt("player", pPlayerAttacker->GetUserID());
					gameeventmanager->FireEvent(pEvent, true);
				}
			}
		}
	}

	FireDeathOutput( info.GetInflictor() );
	
	BaseClass::Event_Killed( info );
}


//-----------------------------------------------------------------------------------------------------
void CTFFlyingMob::UpdateOnRemove()
{
	CPVSFilter filter( GetAbsOrigin() );
	UserMessageBegin( filter, "BreakModel" );
		WRITE_SHORT( GetModelIndex() );
		WRITE_VEC3COORD( GetAbsOrigin() );
		WRITE_ANGLES( GetAbsAngles() );
		WRITE_SHORT( m_nSkin );
	MessageEnd();

	UTIL_Remove( m_hHat );

	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------------------------------
void CTFFlyingMob::FireDeathOutput( CBaseEntity *pCulprit )
{
	// only fire this once
	if ( m_bDeathOutputFired )
		return;
	
	m_bDeathOutputFired = true;
	m_OnDeath.FireOutput( pCulprit, this );
}


//---------------------------------------------------------------------------------------------
/*static*/ CTFFlyingMob* CTFFlyingMob::SpawnAtPos( const Vector& vSpawnPos, const Vector& vFaceTowards, CBaseEntity *pOwner /*= NULL*/, MobType_t nMobType /*= SKELETON_NORMAL*/, float flLifeTime /*= 0.f*/, int nTeam /*= TF_TEAM_HALLOWEEN*/ )
{
	CTFFlyingMob *pFlyingMob = (CTFFlyingMob *)CreateEntityByName( "tf_flying_mob" );
	if ( pFlyingMob )
	{
		pFlyingMob->ChangeTeam( nTeam );

		if (pOwner)
			pFlyingMob->SetOwnerEntity( pOwner );		

		CTFNavArea *pNav = (CTFNavArea *)TheNavMesh->GetNearestNavArea( vSpawnPos );
		if ( pNav )
		{
			Vector vNavPos;
			pNav->GetClosestPointOnArea( vSpawnPos, &vNavPos );
			pFlyingMob->SetAbsOrigin( vNavPos );
		}
		else
		{
			pFlyingMob->SetAbsOrigin( vSpawnPos );
		}

		DispatchSpawn( pFlyingMob );

		if ( flLifeTime > 0.0f )
		{
			pFlyingMob->StartLifeTimer( flLifeTime );
		}

		pFlyingMob->SetMobType( nMobType );
	}

	return pFlyingMob;
}


bool CTFFlyingMob::ShouldSuicide() const
{
	// out of life time
	if ( m_lifeTimer.HasStarted() && m_lifeTimer.IsElapsed() )
		return true;

	// owner changed team
	if ( GetOwnerEntity() && GetOwnerEntity()->GetTeamNumber() != GetTeamNumber() )
		return true;

	return m_bForceSuicide;
}


//-----------------------------------------------------------------------------------------------------
void CTFFlyingMob::SetMobType( MobType_t nType )
{
	m_nType = nType;

	switch ( m_nType )
	{
		case MobType_t::FLYING_NORMAL:
		default:
		{
			m_flHeadScale = 1.f;

			m_flAttackRange = 50.f;
			m_flAttackDamage = 30.f;

			m_flSpecialAttackRange = 500.0f;
			m_flSpecialAttackDamage = 50.0f;

			SetModel( FLYING_MOB_MODEL );
			SetModelScale( 1.0f );
			SetHealth( 50.0f );
			SetMaxHealth( 50.0f );

			if ( RandomFloat( 0.0f, 1.0f ) < 0.3f )
			{
				int iModelIndex = RandomInt( 0, ARRAYSIZE( s_mobHatModels ) - 1 );
				const char *pszHat = s_mobHatModels[ iModelIndex ];
				AddHat( pszHat );
			}

			break;
		}
	}
}

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CTFFlyingMobBehavior : public Action< CTFFlyingMob >
{
public:
	virtual Action< CTFFlyingMob > *InitialContainedAction( CTFFlyingMob *me )	
	{
		return new CTFFlyingMobSpawn;
	}

	virtual ActionResult< CTFFlyingMob >	OnStart( CTFFlyingMob *me, Action< CTFFlyingMob > *priorAction )
	{
		return Continue();
	}

	virtual ActionResult< CTFFlyingMob >	Update( CTFFlyingMob *me, float interval )
	{
		bool bDead = !me->IsAlive();
		if ( !bDead && me->ShouldSuicide() )
		{
			me->FireDeathOutput( me );
			bDead = true;
		}

		if ( bDead )
		{
			UTIL_Remove( me );
			return Done();
		}

		if ( ShouldLaugh( me ) )
		{
			Laugh( me );
		}

		return Continue();
	}

	virtual EventDesiredResult< CTFFlyingMob > OnKilled( CTFFlyingMob *me, const CTakeDamageInfo &info )
	{
		UTIL_Remove( me );

		return TryDone();
	}

	virtual const char *GetName( void ) const	{ return "FlyingMobBehavior"; }		// return name of this action

private:

	bool ShouldLaugh( CTFFlyingMob *me )
	{
		if ( !m_laughTimer.HasStarted() )
		{
			switch ( me->GetMobType() )
			{
			default:
				{
					m_laughTimer.Start( RandomFloat( 4.f, 5.f ) );
				}
			}

			return false;
		}

		if ( m_laughTimer.HasStarted() && m_laughTimer.IsElapsed() )
		{
			m_laughTimer.Invalidate();
			return true;
		}

		return false;
	}

	void Laugh( CTFFlyingMob *me )
	{
		const char *pszSoundName;
		switch ( me->GetMobType() )
		{
			default:
			{
				pszSoundName = RandomInt(0, 1) == 0 ? "Halloween.EyeballBossLaugh" : "Halloween.EyeballBossBigLaugh";
				break;
			}
		}

		me->EmitSound( pszSoundName );
	}

	CountdownTimer m_laughTimer;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CTFFlyingMobIntention::CTFFlyingMobIntention( CTFFlyingMob *me ) : IIntention( me )
{ 
	m_behavior = new Behavior< CTFFlyingMob >( new CTFFlyingMobBehavior ); 
}

CTFFlyingMobIntention::~CTFFlyingMobIntention()
{
	delete m_behavior;
}

void CTFFlyingMobIntention::Reset( void )
{ 
	delete m_behavior; 
	m_behavior = new Behavior< CTFFlyingMob >( new CTFFlyingMobBehavior );
}

void CTFFlyingMobIntention::Update( void )
{
	m_behavior->Update( static_cast< CTFFlyingMob * >( GetBot() ), GetUpdateInterval() ); 
}

// is this a place we can be?
QueryResultType CTFFlyingMobIntention::IsPositionAllowed( const INextBot *meBot, const Vector &pos ) const
{
	return ANSWER_YES;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
float CTFFlyingMobLocomotion::GetRunSpeed( void ) const
{
	return 350.0f;
}


//---------------------------------------------------------------------------------------------
// if delta Z is greater than this, we have to jump to get up
float CTFFlyingMobLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum height of a jump
float CTFFlyingMobLocomotion::GetMaxJumpHeight( void ) const
{
	return 72.0f;
}


//---------------------------------------------------------------------------------------------
// Return max rate of yaw rotation
float CTFFlyingMobLocomotion::GetMaxYawRate( void ) const
{
	return 200.0f;
}


//---------------------------------------------------------------------------------------------
bool CTFFlyingMobLocomotion::ShouldCollideWith( const CBaseEntity *object ) const
{
	return false;
}
