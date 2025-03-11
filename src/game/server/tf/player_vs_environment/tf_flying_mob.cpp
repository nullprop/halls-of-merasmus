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
#include "map_entities/tf_mob_generator.h"

#define FLYING_MOB_MODEL "models/props_halloween/halloween_demoeye.mdl"

ConVar tf_max_active_flying_mobs( "tf_max_active_flying_mobs", "8", FCVAR_CHEAT );

ConVar tf_flying_mob_speed( "tf_flying_mob_speed", "150", FCVAR_CHEAT );
ConVar tf_flying_mob_hover_height( "tf_flying_mob_hover_height", "200", FCVAR_CHEAT );
ConVar tf_flying_mob_acceleration( "tf_flying_mob_acceleration", "300", FCVAR_CHEAT );
ConVar tf_flying_mob_horiz_damping( "tf_flying_mob_horiz_damping", "2", FCVAR_CHEAT );
ConVar tf_flying_mob_vert_damping( "tf_flying_mob_vert_damping", "1", FCVAR_CHEAT );

//-----------------------------------------------------------------------------------------------------
// NPC FlyingMob versions of the players
//-----------------------------------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( tf_flying_mob, CTFFlyingMob );

IMPLEMENT_SERVERCLASS_ST( CTFFlyingMob, DT_TFFlyingMob )
	SendPropExclude( "DT_BaseEntity", "m_angRotation" ),	// client has its own orientation logic
	SendPropExclude( "DT_BaseEntity", "m_angAbsRotation" ),	// client has its own orientation logic
	SendPropVector( SENDINFO( m_lookAtSpot ), 0, SPROP_COORD ),
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
	m_vision = new CTFFlyingMobVision( this );

	m_nType = MobType_t::FLYING_NORMAL;

	m_eyeOffset = vec3_origin;
	m_lookAtSpot = vec3_origin;

	m_flAttackRange = 800.f;
	m_flAttackDamage = 30.f;

	m_bDeathOutputFired = false;
}


//-----------------------------------------------------------------------------------------------------
CTFFlyingMob::~CTFFlyingMob()
{
	if ( m_vision )
		delete m_vision;

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
	PrecacheScriptSound( "Halloween.EyeballBossBecomeAlert" );
	

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

	EmitSound( "Halloween.EyeballBossBecomeAlert" );

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
	DispatchParticleEffect( "eyeboss_death", GetAbsOrigin(), GetAbsAngles() );

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
			cashClass = "item_mobdrop_cash_medium";
			ammoClass = "item_mobdrop_ammo_medium";
			healthClass = "item_mobdrop_health_medium";
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

	FireDeathOutput( info.GetInflictor() );
	
	BaseClass::Event_Killed( info );
}
//-----------------------------------------------------------------------------------------------------
void CTFFlyingMob::FireDeathOutput( CBaseEntity *pCulprit )
{
	// only fire this once
	if ( m_bDeathOutputFired )
		return;
	
	m_bDeathOutputFired = true;
	m_OnDeath.FireOutput( pCulprit, this );

	// Fire tf_mob_generator output
	CTFMobGenerator* pGenerator = (CTFMobGenerator *)GetOwnerEntity();
	if ( pGenerator )
	{
		pGenerator->OnMobKilled( this );
	}
}


//---------------------------------------------------------------------------------------------
/*static*/ CTFFlyingMob* CTFFlyingMob::SpawnAtPos( const Vector& vSpawnPos, const Vector& vFaceTowards, CBaseEntity *pOwner /*= NULL*/, MobType_t nMobType /*= FLYING_NORMAL*/, float flLifeTime /*= 0.f*/, int nTeam /*= TF_TEAM_HALLOWEEN*/ )
{
	CTFFlyingMob *pFlyingMob = (CTFFlyingMob *)CreateEntityByName( "tf_flying_mob" );
	if ( pFlyingMob )
	{
		pFlyingMob->ChangeTeam( nTeam );

		if ( pOwner )
			pFlyingMob->SetOwnerEntity( pOwner );		

		Vector vActualSpawnPos = vSpawnPos;

		CTFNavArea *pNav = (CTFNavArea *)TheNavMesh->GetNearestNavArea( vSpawnPos );
		if ( pNav )
		{
			pNav->GetClosestPointOnArea( vSpawnPos, &vActualSpawnPos );
		}

		vActualSpawnPos.z += tf_flying_mob_hover_height.GetFloat();

		trace_t trace;
		pFlyingMob->GetLocomotionInterface()->TraceHull(
			vActualSpawnPos,
			vActualSpawnPos + Vector( 0, 0, -tf_flying_mob_hover_height.GetFloat() ), 
			pFlyingMob->WorldAlignMins(), pFlyingMob->WorldAlignMaxs(), 
			pFlyingMob->GetBodyInterface()->GetSolidMask(), NULL, &trace
		);
		if ( trace.startsolid && !trace.allsolid )
		{
			vActualSpawnPos = trace.endpos;
		}

		pFlyingMob->SetAbsOrigin( vActualSpawnPos );
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
			m_flAttackRange = 800.f;
			m_flAttackDamage = 30.f;

			SetModel( FLYING_MOB_MODEL );
			SetModelScale( 1.0f );
			SetHealth( 200.0f );
			SetMaxHealth( 200.0f );

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
					m_laughTimer.Start( RandomFloat( 6.f, 8.f ) );
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
CTFFlyingMobLocomotion::CTFFlyingMobLocomotion( INextBot *bot ) : ILocomotion( bot )
{
	Reset();
}


//---------------------------------------------------------------------------------------------
CTFFlyingMobLocomotion::~CTFFlyingMobLocomotion()
{
}


//---------------------------------------------------------------------------------------------
// (EXTEND) reset to initial state
void CTFFlyingMobLocomotion::Reset( void )
{
	m_velocity = vec3_origin;
	m_acceleration = vec3_origin;
	m_desiredSpeed = 0.0f;
	m_currentSpeed = 0.0f;
	m_forward = vec3_origin;
	m_desiredAltitude = tf_flying_mob_hover_height.GetFloat();
}

//---------------------------------------------------------------------------------------------
void CTFFlyingMobLocomotion::MaintainAltitude( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	if ( !me->IsAlive() )
	{
		m_acceleration.x = 0.0f;
		m_acceleration.y = 0.0f;
		m_acceleration.z = -300.0f;

		return;
	}

	trace_t result;
	CTraceFilterSimpleClassnameList filter( me, COLLISION_GROUP_NONE );
	filter.AddClassnameToIgnore( "eyeball_boss" );

	// find ceiling
	TraceHull( me->GetAbsOrigin(), me->GetAbsOrigin() + Vector( 0, 0, 1000.0f ), 
			   me->WorldAlignMins(), me->WorldAlignMaxs(), 
			   GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

	float ceiling = result.endpos.z - me->GetAbsOrigin().z;

	Vector aheadXY;
	
	if ( IsAttemptingToMove() )
	{
		aheadXY.x = m_forward.x;
		aheadXY.y = m_forward.y;
		aheadXY.z = 0.0f;
		aheadXY.NormalizeInPlace();
	}
	else
	{
		aheadXY = vec3_origin;
	}

	TraceHull( me->GetAbsOrigin() + Vector( 0, 0, ceiling ) + aheadXY * 50.0f,
			   me->GetAbsOrigin() + Vector( 0, 0, -2000.0f ) + aheadXY * 50.0f,
			   Vector( 1.25f * me->WorldAlignMins().x, 1.25f * me->WorldAlignMins().y, me->WorldAlignMins().z ), 
			   Vector( 1.25f * me->WorldAlignMaxs().x, 1.25f * me->WorldAlignMaxs().y, me->WorldAlignMaxs().z ), 
			   GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

	float groundZ = result.endpos.z;

	float currentAltitude = me->GetAbsOrigin().z - groundZ;

	float desiredAltitude = GetDesiredAltitude();

	float error = desiredAltitude - currentAltitude;

	float accelZ = clamp( error, -tf_flying_mob_acceleration.GetFloat(), tf_flying_mob_acceleration.GetFloat() );

	m_acceleration.z += accelZ;
}

//---------------------------------------------------------------------------------------------
// (EXTEND) update internal state
void CTFFlyingMobLocomotion::Update( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();
	const float deltaT = GetUpdateInterval();

	Vector pos = me->GetAbsOrigin();

	// always maintain altitude, even if not trying to move (ie: no Approach call)
	MaintainAltitude();

	m_forward = m_velocity;
	m_currentSpeed = m_forward.NormalizeInPlace();

	Vector damping( tf_flying_mob_horiz_damping.GetFloat(), tf_flying_mob_horiz_damping.GetFloat(), tf_flying_mob_vert_damping.GetFloat() );
	Vector totalAccel = m_acceleration - m_velocity * damping;

	m_velocity += totalAccel * deltaT;
	me->SetAbsVelocity( m_velocity );

	pos += m_velocity * deltaT;

	// check for collisions along move	
	trace_t result;
	CTraceFilterSkipClassname filter( me, "tf_flying_mob", COLLISION_GROUP_NONE );
	Vector from = me->GetAbsOrigin();
	Vector to = pos;
	Vector desiredGoal = to;
	Vector resolvedGoal;
	int recursionLimit = 3;

	int hitCount = 0;
	Vector surfaceNormal = vec3_origin;

	bool didHitWorld = false;

	while( true )
	{
		TraceHull( from, desiredGoal, me->WorldAlignMins(), me->WorldAlignMaxs(), GetBot()->GetBodyInterface()->GetSolidMask(), &filter, &result );

		if ( !result.DidHit() )
		{
			resolvedGoal = pos;
			break;
		}

		if ( result.DidHitWorld() )
		{
			didHitWorld = true;
		}

		++hitCount;
		surfaceNormal += result.plane.normal;

		// If we hit really close to our target, then stop
		if ( !result.startsolid && desiredGoal.DistToSqr( result.endpos ) < 1.0f )
		{
			resolvedGoal = result.endpos;
			break;
		}

		if ( result.startsolid )
		{
			// stuck inside solid; don't move
			resolvedGoal = me->GetAbsOrigin();
			break;
		}

		if ( --recursionLimit <= 0 )
		{
			// reached recursion limit, no more adjusting allowed
			resolvedGoal = result.endpos;
			break;
		}

		// slide off of surface we hit
		Vector fullMove = desiredGoal - from;
		Vector leftToMove = fullMove * ( 1.0f - result.fraction );

		float blocked = DotProduct( result.plane.normal, leftToMove );

		Vector unconstrained = fullMove - blocked * result.plane.normal;

		// check for collisions along remainder of move
		// But don't bother if we're not going to deflect much
		Vector remainingMove = from + unconstrained;
		if ( remainingMove.DistToSqr( result.endpos ) < 1.0f )
		{
			resolvedGoal = result.endpos;
			break;
		}

		desiredGoal = remainingMove;
	}

	if ( hitCount > 0 )
	{
		surfaceNormal.NormalizeInPlace();

		// bounce
		m_velocity = m_velocity - 2.0f * DotProduct( m_velocity, surfaceNormal ) * surfaceNormal;

		if ( didHitWorld )
		{
			//me->EmitSound( "Minion.Bounce" );
		}
	}

	GetBot()->GetEntity()->SetAbsOrigin( result.endpos );

	m_acceleration = vec3_origin;
}


//---------------------------------------------------------------------------------------------
// (EXTEND) move directly towards the given position
void CTFFlyingMobLocomotion::Approach( const Vector &goalPos, float goalWeight )
{
	Vector flyGoal = goalPos;
	flyGoal.z += m_desiredAltitude;

	Vector toGoal = flyGoal - GetBot()->GetEntity()->GetAbsOrigin();
	// altitude is handled in Update()
	toGoal.z = 0.0f;
	toGoal.NormalizeInPlace();

	m_acceleration += tf_flying_mob_acceleration.GetFloat() * toGoal;
}


//---------------------------------------------------------------------------------------------
void CTFFlyingMobLocomotion::SetDesiredSpeed( float speed )
{
	m_desiredSpeed = speed;
}


//---------------------------------------------------------------------------------------------
float CTFFlyingMobLocomotion::GetDesiredSpeed( void ) const
{
	return m_desiredSpeed;
}


//---------------------------------------------------------------------------------------------
void CTFFlyingMobLocomotion::SetDesiredAltitude( float height )
{
	m_desiredAltitude = height;
}


//---------------------------------------------------------------------------------------------
float CTFFlyingMobLocomotion::GetDesiredAltitude( void ) const
{
	return m_desiredAltitude;
}


//---------------------------------------------------------------------------------------------
// Face along path. Since we float, only face horizontally.
void CTFFlyingMobLocomotion::FaceTowards( const Vector &target )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	Vector toTarget = target - me->WorldSpaceCenter();
	toTarget.z = 0.0f;

	QAngle angles;
	VectorAngles( toTarget, angles );

	me->SetAbsAngles( angles );
}


//---------------------------------------------------------------------------------------------
// return position of "feet" - the driving point where the bot contacts the ground
// for this floating boss, "feet" refers to the ground directly underneath him
const Vector &CTFFlyingMobLocomotion::GetFeet( void ) const
{
	static Vector feet;
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	trace_t result;
	CTraceFilterSimpleClassnameList filter( me, COLLISION_GROUP_NONE );
	filter.AddClassnameToIgnore( "tf_flying_mob" );

	feet = me->GetAbsOrigin();

	UTIL_TraceLine( feet, feet + Vector( 0, 0, -2000.0f ), MASK_PLAYERSOLID_BRUSHONLY, &filter, &result );

	feet.z = result.endpos.z;

	return feet;
}