//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_FLYING_MOB_H
#define TF_FLYING_MOB_H

#include "NextBot.h"
#include "NextBotBehavior.h"
#include "NextBotGroundLocomotion.h"
#include "Path/NextBotPathFollow.h"
#include "tf_flying_mob_body.h"
#include "player_vs_environment/tf_mob_common.h"

class CTFFlyingMob;


class CTFFlyingMobVision : public IVision
{
public:
	CTFFlyingMobVision( INextBot *bot ) : IVision( bot ) { }
	virtual ~CTFFlyingMobVision() { }

	virtual void Reset( void )	{ }
	virtual void Update( void ) { }
};

//----------------------------------------------------------------------------
class CTFFlyingMobLocomotion : public ILocomotion
{
public:
	CTFFlyingMobLocomotion( INextBot *bot );
	virtual ~CTFFlyingMobLocomotion();

	virtual void Reset( void );								// (EXTEND) reset to initial state
	virtual void Update( void );							// (EXTEND) update internal state

	virtual void Approach( const Vector &goalPos, float goalWeight = 1.0f );	// (EXTEND) move directly towards the given position

	virtual void SetDesiredSpeed( float speed );			// set desired speed for locomotor movement
	virtual float GetDesiredSpeed( void ) const;			// returns the current desired speed

	virtual float GetStepHeight( void ) const;				// if delta Z is greater than this, we have to jump to get up
	virtual float GetMaxJumpHeight( void ) const;			// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;			// distance at which we will die if we fall

	virtual void SetDesiredAltitude( float height );		// how high above our Approach goal do we float?
	virtual float GetDesiredAltitude( void ) const;

	virtual const Vector &GetGroundNormal( void ) const;	// surface normal of the ground we are in contact with

	virtual const Vector &GetVelocity( void ) const;		// return current world space velocity
	void SetVelocity( const Vector &velocity );

	virtual void FaceTowards( const Vector &target );		// rotate body to face towards "target"

	// return position of "feet" - the driving point where the bot contacts the ground
	// for this floating boss, "feet" refers to the ground directly underneath him
	virtual const Vector &GetFeet( void ) const;			

protected:
	float m_desiredSpeed;
	float m_currentSpeed;
	Vector m_forward;

	float m_desiredAltitude;
	void MaintainAltitude( void );

	Vector m_velocity;
	Vector m_acceleration;
};


inline float CTFFlyingMobLocomotion::GetStepHeight( void ) const
{
	return 50.0f;
}

inline float CTFFlyingMobLocomotion::GetMaxJumpHeight( void ) const
{
	return 100.0f;
}

inline float CTFFlyingMobLocomotion::GetDeathDropHeight( void ) const
{
	return 999.9f;
}

inline const Vector &CTFFlyingMobLocomotion::GetGroundNormal( void ) const
{
	static Vector up( 0, 0, 1.0f );

	return up;
}

inline const Vector &CTFFlyingMobLocomotion::GetVelocity( void ) const
{
	return m_velocity;
}

inline void CTFFlyingMobLocomotion::SetVelocity( const Vector &velocity )
{
	m_velocity = velocity;
}


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

	// INextBot
	virtual CTFFlyingMobIntention	*GetIntentionInterface( void ) const	{ return m_intention; }
	virtual CTFFlyingMobLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CTFFlyingMobBody	*GetBodyInterface( void ) const			{ return m_body; }

	virtual Vector EyePosition( void );
	virtual void SetLookAtTarget( const Vector &spot );

	void StartLifeTimer( float flLifeTime ) { m_lifeTimer.Start( flLifeTime ); }
	bool ShouldSuicide() const;
	void ForceSuicide() { m_bForceSuicide = true; }

	MobType_t GetMobType() const { return m_nType; }
	void SetMobType( MobType_t nType );
	void AddHat( const char *pszModel );

	static CTFFlyingMob* SpawnAtPos( const Vector& vSpawnPos, const Vector& vFaceTowards, CBaseEntity *pOwner = NULL, MobType_t nSkeletonType = FLYING_NORMAL, float flLifeTime = 0.f, int nTeam = TF_TEAM_HALLOWEEN );

	float GetAttackRange() const { return m_flAttackRange; }
	float GetAttackDamage() const { return m_flAttackDamage; }

	void FireDeathOutput( CBaseEntity *pCulprit );
private:
	CTFFlyingMobIntention *m_intention;
	CTFFlyingMobLocomotion *m_locomotor;
	CTFFlyingMobBody *m_body;
	CTFFlyingMobVision *m_vision;

	MobType_t m_nType;

	Vector m_eyeOffset;

	CNetworkVector( m_lookAtSpot );

	float m_flAttackRange;
	float m_flAttackDamage;

	bool m_bForceSuicide;
	CountdownTimer m_lifeTimer;

	bool m_bDeathOutputFired;
	COutputEvent m_OnDeath;
};

inline Vector CTFFlyingMob::EyePosition( void )
{
	return GetAbsOrigin() + m_eyeOffset;
}

inline void CTFFlyingMob::SetLookAtTarget( const Vector &spot )
{
	m_lookAtSpot = spot;
}

#endif // TF_FLYING_MOB_H
