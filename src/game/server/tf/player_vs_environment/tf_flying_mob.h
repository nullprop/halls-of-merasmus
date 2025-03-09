//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_GROUND_MOB_H
#define TF_GROUND_MOB_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"
#include "tf_flying_mob_body.h"
#include "tf_mob_common.h"

class CTFFlyingMob;


//----------------------------------------------------------------------------
class CTFFlyingMobLocomotion : public CNextBotFlyingLocomotion
{
public:
	CTFFlyingMobLocomotion( INextBot *bot ) : CNextBotFlyingLocomotion( bot ) { }
	virtual ~CTFFlyingMobLocomotion() { }

	virtual void SetDesiredSpeed( float speed );			// set desired speed for locomotor movement
	virtual float GetDesiredSpeed( void ) const;			// returns the current desired speed

	virtual void SetDesiredAltitude( float height );		// how high above our Approach goal do we float?
	virtual float GetDesiredAltitude( void ) const;

	virtual bool ShouldCollideWith( const CBaseEntity *object ) const;

private:
	virtual float GetMaxYawRate( void ) const;				// return max rate of yaw rotation
};


//----------------------------------------------------------------------------
class CTFFlyingMobIntention : public IIntention
{
public:
	CTFFlyingMobIntention( CTFFlyingMob *me );
	virtual ~CTFFlyingMobIntention();

	virtual void Reset( void );
	virtual void Update( void );

	virtual QueryResultType			IsPositionAllowed( const INextBot *me, const Vector &pos ) const;	// is the a place we can be?

	virtual INextBotEventResponder *FirstContainedResponder( void ) const  { return m_behavior; }
	virtual INextBotEventResponder *NextContainedResponder( INextBotEventResponder *current ) const { return NULL; }

private:
	Behavior< CTFFlyingMob > *m_behavior;
};


//----------------------------------------------------------------------------
DECLARE_AUTO_LIST( ITFFlyingMobAutoList );

class CTFFlyingMob : public NextBotCombatCharacter, public ITFFlyingMobAutoList
{
public:
	DECLARE_CLASS( CTFFlyingMob, NextBotCombatCharacter );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CTFFlyingMob();
	virtual ~CTFFlyingMob();

	static void PrecacheFlyingMob();
	virtual void Precache();
	virtual void Spawn( void );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void UpdateOnRemove();

	// INextBot
	virtual CTFFlyingMobIntention	*GetIntentionInterface( void ) const	{ return m_intention; }
	virtual CTFFlyingMobLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CTFFlyingMobBody	*GetBodyInterface( void ) const			{ return m_body; }

	CBaseAnimating *m_zombieParts;

	void StartLifeTimer( float flLifeTime ) { m_lifeTimer.Start( flLifeTime ); }
	bool ShouldSuicide() const;
	void ForceSuicide() { m_bForceSuicide = true; }

	MobType_t GetMobType() const { return m_nType; }
	void SetMobType( MobType_t nType );
	void AddHat( const char *pszModel );

	static CTFFlyingMob* SpawnAtPos( const Vector& vSpawnPos, const Vector& vFaceTowards, CBaseEntity *pOwner = NULL, MobType_t nSkeletonType = FLYING_NORMAL, float flLifeTime = 0.f, int nTeam = TF_TEAM_HALLOWEEN );

	float GetAttackRange() const { return m_flAttackRange; }
	float GetAttackDamage() const { return m_flAttackDamage; }
	float GetSpecialAttackRange() const { return m_flSpecialAttackRange; }
	float GetSpecialAttackDamage() const { return m_flSpecialAttackDamage; }

	void FireDeathOutput( CBaseEntity *pCulprit );
private:
	CTFFlyingMobIntention *m_intention;
	CTFFlyingMobLocomotion *m_locomotor;
	CTFFlyingMobBody *m_body;

	MobType_t m_nType;
	CNetworkVar( float, m_flHeadScale );

	CHandle< CBaseAnimating > m_hHat;

	struct RecentDamager_t
	{
		EHANDLE m_hEnt;
		float m_flDamageTime;
	};

	float m_flAttackRange;
	float m_flAttackDamage;
	float m_flSpecialAttackRange;
	float m_flSpecialAttackDamage;

	bool m_bForceSuicide;
	CountdownTimer m_lifeTimer;

	bool m_bDeathOutputFired;
	COutputEvent m_OnDeath;
};



//--------------------------------------------------------------------------------------------------------------
class CTFFlyingMobPathCost : public IPathCost
{
public:
	CTFFlyingMobPathCost( CTFFlyingMob *me )
	{
		m_me = me;
	}

	// return the cost (weighted distance between) of moving from "fromArea" to "area", or -1 if the move is not allowed
	virtual float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
	{
		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			if ( !m_me->GetLocomotionInterface()->IsAreaTraversable( area ) )
			{
				// our locomotor says we can't move here
				return -1.0f;
			}

			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				// optimization to avoid recomputing length
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}

			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange( area );
			if ( deltaZ >= m_me->GetLocomotionInterface()->GetStepHeight() )
			{
				if ( deltaZ >= m_me->GetLocomotionInterface()->GetMaxJumpHeight() )
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = 5.0f;
				dist += jumpPenalty * dist;
			}
			else if ( deltaZ < -m_me->GetLocomotionInterface()->GetDeathDropHeight() )
			{
				// too far to drop
				return -1.0f;
			}

			// this term causes the same bot to choose different routes over time,
			// but keep the same route for a period in case of repaths
			int timeMod = (int)( gpGlobals->curtime / 10.0f ) + 1;
			float preference = 1.0f + 50.0f * ( 1.0f + FastCos( (float)( m_me->GetEntity()->entindex() * area->GetID() * timeMod ) ) );
			float cost = dist * preference;

			return cost + fromArea->GetCostSoFar();
		}
	}

	CTFFlyingMob *m_me;
};


#endif // TF_GROUND_MOB_H
