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
#include "tf_ammo_pack.h"
#include "entity_healthkit.h"

#include "tf_melee_mob.h"
#include "mob_behavior/melee_mob_spawn.h"

#include "halloween/tf_weapon_spellbook.h"

#define MELEE_MOB_MODEL "models/bots/skeleton_sniper/skeleton_sniper.mdl"
#define MELEE_GIANT_MOB_MODEL "models/bots/skeleton_sniper_boss/skeleton_sniper_boss.mdl"
#define SKELETON_KING_CROWN_MODEL "models/player/items/demo/crown.mdl"

ConVar tf_max_active_mobs( "tf_max_active_mobs", "64", FCVAR_CHEAT );


//-----------------------------------------------------------------------------------------------------
// NPC MeleeMob versions of the players
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_melee_mob, CTFMeleeMob );

IMPLEMENT_SERVERCLASS_ST( CTFMeleeMob, DT_TFMeleeMob )
	SendPropFloat( SENDINFO( m_flHeadScale ) ),
END_SEND_TABLE()

IMPLEMENT_AUTO_LIST( ITFMeleeMobAutoList );


static const char *s_mobHatModels[] =
{
	"models/player/items/all_class/skull_scout.mdl",
	"models/workshop/player/items/scout/hw2013_boston_bandy_mask/hw2013_boston_bandy_mask.mdl",
	"models/workshop/player/items/demo/hw2013_blackguards_bicorn/hw2013_blackguards_bicorn.mdl",
	"models/player/items/heavy/heavy_big_chief.mdl",
};

BEGIN_DATADESC( CTFMeleeMob )
	DEFINE_OUTPUT( m_OnDeath, "OnDeath" ),
END_DATADESC()


//-----------------------------------------------------------------------------------------------------
CTFMeleeMob::CTFMeleeMob()
{
	m_intention = new CTFMeleeMobIntention( this );
	m_locomotor = new CTFMeleeMobLocomotion( this );
	m_body = new CTFMeleeMobBody( this );

	m_nType = CTFMeleeMob::MobType_t::MOB_NORMAL;

	m_flHeadScale = 1.f;

	m_flAttackRange = 50.f;
	m_flAttackDamage = 30.f;
	m_flSpecialAttackRange = 500.f;
	m_flSpecialAttackDamage = 50.f;

	m_bDeathOutputFired = false;
}


//-----------------------------------------------------------------------------------------------------
CTFMeleeMob::~CTFMeleeMob()
{
	if ( m_intention )
		delete m_intention;

	if ( m_locomotor )
		delete m_locomotor;

	if ( m_body )
		delete m_body;
}


void CTFMeleeMob::PrecacheMeleeMob()
{
	int nSkeletonModel = PrecacheModel( MELEE_MOB_MODEL );
	PrecacheGibsForModel( nSkeletonModel );

	int nSkeletonKingModel = PrecacheModel( MELEE_GIANT_MOB_MODEL );
	PrecacheGibsForModel( nSkeletonKingModel );

	PrecacheModel( SKELETON_KING_CROWN_MODEL );

	for ( int i=0; i<ARRAYSIZE( s_mobHatModels ) ; ++i )
	{
		PrecacheModel( s_mobHatModels[i] );
	}

	PrecacheParticleSystem( "bomibomicon_ring" );
	PrecacheParticleSystem( "spell_pumpkin_mirv_goop_red" );
	PrecacheParticleSystem( "spell_pumpkin_mirv_goop_blue" );
	PrecacheParticleSystem( "spell_skeleton_goop_green" );

	PrecacheScriptSound( "Halloween.skeleton_break" );
	PrecacheScriptSound( "Halloween.skeleton_laugh_small" );
	PrecacheScriptSound( "Halloween.skeleton_laugh_medium" );
	PrecacheScriptSound( "Halloween.skeleton_laugh_giant" );
}


//-----------------------------------------------------------------------------------------------------
void CTFMeleeMob::Precache()
{
	BaseClass::Precache();

	bool bAllowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	PrecacheMeleeMob();

	CBaseEntity::SetAllowPrecache( bAllowPrecache );
}


//-----------------------------------------------------------------------------------------------------
void CTFMeleeMob::Spawn( void )
{
	Precache();

	BaseClass::Spawn();

	AddFlag( FL_NPC );

	QAngle qAngle = vec3_angle;
	qAngle[YAW] = RandomFloat( 0, 360 );
	SetAbsAngles( qAngle );

	// Spawn Pos
	GetBodyInterface()->StartActivity( ACT_TRANSITION );

	switch ( GetTeamNumber() )
	{
	case TF_TEAM_RED:
		m_nSkin = 0;
		break;
	case TF_TEAM_BLUE:
		m_nSkin = 1;
		break;
	default:
		{
			m_nSkin = 2;
			// make sure I'm on TF_TEAM_HALLOWEEN
			ChangeTeam( TF_TEAM_HALLOWEEN );
		}
	}

	// force kill oldest skeletons in the level (except skeleton king) to keep the number of skeletons under the max active
	int nForceKill = ITFMeleeMobAutoList::AutoList().Count() - tf_max_active_mobs.GetInt();
	for ( int i=0; i<ITFMeleeMobAutoList::AutoList().Count() && nForceKill > 0; ++i )
	{
		CTFMeleeMob *pMeleeMob = static_cast< CTFMeleeMob* >( ITFMeleeMobAutoList::AutoList()[i] );
		pMeleeMob->ForceSuicide();
		nForceKill--;
	}
	Assert( nForceKill <= 0 );
}


//-----------------------------------------------------------------------------------------------------
int CTFMeleeMob::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( info.GetAttacker() && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() )
		return 0;

	if ( !IsPlayingGesture( ACT_MP_GESTURE_FLINCH_CHEST ) )
	{
		AddGesture( ACT_MP_GESTURE_FLINCH_CHEST );
	}

	const char* pszEffectName;
	if ( GetTeamNumber() == TF_TEAM_HALLOWEEN )
	{
		pszEffectName = "spell_skeleton_goop_green";
	}
	else
	{
		pszEffectName = GetTeamNumber() == TF_TEAM_RED ? "spell_pumpkin_mirv_goop_red" : "spell_pumpkin_mirv_goop_blue";
	}

	DispatchParticleEffect( pszEffectName, info.GetDamagePosition(), GetAbsAngles() );

	return BaseClass::OnTakeDamage_Alive( info );
}


//-----------------------------------------------------------------------------------------------------
void CTFMeleeMob::Event_Killed( const CTakeDamageInfo &info )
{
	EmitSound( "Halloween.skeleton_break" );

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
		case CTFMeleeMob::MobType_t::MOB_NORMAL:
		default:
		{
			cashAmount = 10;
			ammoAmount = 4;
			healthAmount = 20;
			cashClass = "item_mobdrop_cash_small";
			ammoClass = "item_mobdrop_ammo_small";
			healthClass = "item_mobdrop_health_small";
			break;
		}

		case CTFMeleeMob::MobType_t::MOB_GIANT:
		{
			cashAmount = 50;
			ammoAmount = 10;
			healthAmount = 100;
			cashClass = "item_mobdrop_cash_large";
			ammoClass = "item_mobdrop_ammo_large";
			healthClass = "item_mobdrop_health_large";
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
void CTFMeleeMob::UpdateOnRemove()
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
void CTFMeleeMob::FireDeathOutput( CBaseEntity *pCulprit )
{
	// only fire this once
	if ( m_bDeathOutputFired )
		return;
	
	m_bDeathOutputFired = true;
	m_OnDeath.FireOutput( pCulprit, this );
}


//---------------------------------------------------------------------------------------------
/*static*/ CTFMeleeMob* CTFMeleeMob::SpawnAtPos( const Vector& vSpawnPos, const Vector& vFaceTowards, CBaseEntity *pOwner /*= NULL*/, MobType_t nMobType /*= SKELETON_NORMAL*/, float flLifeTime /*= 0.f*/, int nTeam /*= TF_TEAM_HALLOWEEN*/ )
{
	CTFMeleeMob *pMeleeMob = (CTFMeleeMob *)CreateEntityByName( "tf_melee_mob" );
	if ( pMeleeMob )
	{
		pMeleeMob->ChangeTeam( nTeam );

		if (pOwner)
			pMeleeMob->SetOwnerEntity( pOwner );		

		CTFNavArea *pNav = (CTFNavArea *)TheNavMesh->GetNearestNavArea( vSpawnPos );
		if ( pNav )
		{
			Vector vNavPos;
			pNav->GetClosestPointOnArea( vSpawnPos, &vNavPos );
			pMeleeMob->SetAbsOrigin( vNavPos );
		}
		else
		{
			pMeleeMob->SetAbsOrigin( vSpawnPos );
		}

		DispatchSpawn( pMeleeMob );

		if ( flLifeTime > 0.0f )
		{
			pMeleeMob->StartLifeTimer( flLifeTime );
		}

		pMeleeMob->SetMobType( nMobType );
	}

	return pMeleeMob;
}


bool CTFMeleeMob::ShouldSuicide() const
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
void CTFMeleeMob::SetMobType( MobType_t nType )
{
	m_nType = nType;

	switch ( m_nType )
	{
		case MOB_NORMAL:
		default:
		{
			m_flHeadScale = 1.f;

			m_flAttackRange = 50.f;
			m_flAttackDamage = 30.f;

			m_flSpecialAttackRange = 500.0f;
			m_flSpecialAttackDamage = 50.0f;

			SetModel( MELEE_MOB_MODEL );
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

		case MOB_GIANT:
		{
			m_flHeadScale = 1.f;

			m_flAttackRange = 60.f;
			m_flAttackDamage = 50.f;

			m_flSpecialAttackRange = 250.0f;
			m_flSpecialAttackDamage = 80.0f;

			SetModel( MELEE_GIANT_MOB_MODEL );
			SetModelScale( 2.0f );
			SetHealth( 500.0f );
			SetMaxHealth( 500.0f );

			AddHat( SKELETON_KING_CROWN_MODEL );

			break;
		}
	}
}


//-----------------------------------------------------------------------------------------------------
void CTFMeleeMob::AddHat( const char *pszModel )
{
	if ( !m_hHat )
	{
		int iHead = LookupBone( "bip_head" );
		Assert( iHead != -1 );
		if ( iHead != -1 )
		{
			m_hHat = (CBaseAnimating *)CreateEntityByName( "prop_dynamic" );
			if ( m_hHat )
			{
				m_hHat->SetModel( pszModel );

				Vector pos;
				QAngle angles;
				GetBonePosition( iHead, pos, angles );
				m_hHat->SetAbsOrigin( pos );
				m_hHat->SetAbsAngles( angles );
				m_hHat->FollowEntity( this, true );
			}
		}
	}
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CTFMeleeMobBehavior : public Action< CTFMeleeMob >
{
public:
	virtual Action< CTFMeleeMob > *InitialContainedAction( CTFMeleeMob *me )	
	{
		return new CTFMeleeMobSpawn;
	}

	virtual ActionResult< CTFMeleeMob >	OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction )
	{
		return Continue();
	}

	virtual ActionResult< CTFMeleeMob >	Update( CTFMeleeMob *me, float interval )
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

	virtual EventDesiredResult< CTFMeleeMob > OnKilled( CTFMeleeMob *me, const CTakeDamageInfo &info )
	{
		// bonemerged models don't ragdoll
		//UTIL_Remove( me->m_zombieParts );

		/*
		if ( info.GetAttacker() && dynamic_cast< CBaseCombatCharacter* >( info.GetAttacker() ) )
		{
			if ( me->GetMobType() == CTFMeleeMob::MOB_NORMAL )
			{
				// normal skeleton spawns 3 mini skeletons
				CBaseCombatCharacter* pOwner = dynamic_cast< CBaseCombatCharacter* >( me->GetOwnerEntity() );
				pOwner = pOwner ? pOwner : me;
				for ( int i=0; i<3; ++i )
				{
					CreateSpellSpawnMeleeMob( pOwner, me->GetAbsOrigin(), 2 );
				}
			}
		}
			*/

		UTIL_Remove( me );

		return TryDone();
	}

	virtual const char *GetName( void ) const	{ return "MeleeMobBehavior"; }		// return name of this action

private:

	bool ShouldLaugh( CTFMeleeMob *me )
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

	void Laugh( CTFMeleeMob *me )
	{
		const char *pszSoundName;
		switch ( me->GetMobType() )
		{
			default:
			{
				pszSoundName = "Halloween.skeleton_laugh_medium";
				break;
			}

			case CTFMeleeMob::MOB_GIANT:
			{
				pszSoundName = "Halloween.skeleton_laugh_giant";
				break;
			}
		}

		me->EmitSound( pszSoundName );
	}

	CountdownTimer m_laughTimer;
};


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CTFMeleeMobIntention::CTFMeleeMobIntention( CTFMeleeMob *me ) : IIntention( me )
{ 
	m_behavior = new Behavior< CTFMeleeMob >( new CTFMeleeMobBehavior ); 
}

CTFMeleeMobIntention::~CTFMeleeMobIntention()
{
	delete m_behavior;
}

void CTFMeleeMobIntention::Reset( void )
{ 
	delete m_behavior; 
	m_behavior = new Behavior< CTFMeleeMob >( new CTFMeleeMobBehavior );
}

void CTFMeleeMobIntention::Update( void )
{
	m_behavior->Update( static_cast< CTFMeleeMob * >( GetBot() ), GetUpdateInterval() ); 
}

// is this a place we can be?
QueryResultType CTFMeleeMobIntention::IsPositionAllowed( const INextBot *meBot, const Vector &pos ) const
{
	return ANSWER_YES;
}



//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
float CTFMeleeMobLocomotion::GetRunSpeed( void ) const
{
	return 350.0f;
}


//---------------------------------------------------------------------------------------------
// if delta Z is greater than this, we have to jump to get up
float CTFMeleeMobLocomotion::GetStepHeight( void ) const
{
	return 18.0f;
}


//---------------------------------------------------------------------------------------------
// return maximum height of a jump
float CTFMeleeMobLocomotion::GetMaxJumpHeight( void ) const
{
	return 72.0f;
}


//---------------------------------------------------------------------------------------------
// Return max rate of yaw rotation
float CTFMeleeMobLocomotion::GetMaxYawRate( void ) const
{
	return 200.0f;
}


//---------------------------------------------------------------------------------------------
bool CTFMeleeMobLocomotion::ShouldCollideWith( const CBaseEntity *object ) const
{
	return false;
}
