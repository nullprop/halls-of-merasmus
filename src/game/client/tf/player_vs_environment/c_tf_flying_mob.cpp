//========= Copyright Valve Corporation, All rights reserved. ============//
// c_eyeball_boss.cpp

#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_tf_flying_mob.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#undef NextBot



//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_TFFlyingMob, DT_TFFlyingMob, CTFFlyingMob )

	RecvPropVector( RECVINFO( m_lookAtSpot ) ),
	
END_RECV_TABLE()


//-----------------------------------------------------------------------------
C_TFFlyingMob::C_TFFlyingMob()
{
	m_auraEffect = NULL;
}


//-----------------------------------------------------------------------------
C_TFFlyingMob::~C_TFFlyingMob()
{
	if ( m_auraEffect )
	{
		ParticleProp()->StopEmission( m_auraEffect );
		m_auraEffect = NULL;
	}
}


//-----------------------------------------------------------------------------
void C_TFFlyingMob::Spawn( void )
{
	BaseClass::Spawn();

	m_leftRightPoseParameter = -1;
	m_upDownPoseParameter = -1;

	m_myAngles = vec3_angle;

	SetNextClientThink( CLIENT_THINK_ALWAYS );
}

void C_TFFlyingMob::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		// eyeboss_aura_calm, eyeboss_aura_grumpy, eyeboss_aura_angry
		m_auraEffect = ParticleProp()->Create( "eyeboss_aura_calm", PATTACH_ABSORIGIN_FOLLOW );
	}
}

//-----------------------------------------------------------------------------
void C_TFFlyingMob::ClientThink( void )
{
	// update eyeball aim
	if ( m_leftRightPoseParameter < 0 )
	{
		m_leftRightPoseParameter = LookupPoseParameter( "left_right" );
	}

	if ( m_upDownPoseParameter < 0 )
	{
		m_upDownPoseParameter = LookupPoseParameter( "up_down" );
	}


	Vector myForward, myRight, myUp;
	AngleVectors( m_myAngles, &myForward, &myRight, &myUp );


	const float myApproachRate = 3.0f; // 1.0f;

	Vector toTarget = m_lookAtSpot - WorldSpaceCenter();
	toTarget.NormalizeInPlace();

	myForward += toTarget * myApproachRate * gpGlobals->frametime;
	myForward.NormalizeInPlace();

	QAngle myNewAngles;
	VectorAngles( myForward, myNewAngles );

	SetAbsAngles( myNewAngles );
	m_myAngles = myNewAngles;



	// set pose parameters to aim pupil directly at target
	float toTargetRight = DotProduct( myRight, toTarget );
	float toTargetUp = DotProduct( myUp, toTarget );

	if ( m_leftRightPoseParameter >= 0 )
	{
		int angle = -50 * toTargetRight;

		SetPoseParameter( m_leftRightPoseParameter, angle );
	}

	if ( m_upDownPoseParameter >= 0 )
	{
		int angle = -50 * toTargetUp;

		SetPoseParameter( m_upDownPoseParameter, angle );
	}
}


//-----------------------------------------------------------------------------
void C_TFFlyingMob::SetDormant( bool bDormant )
{
	BaseClass::SetDormant( bDormant );
}


//-----------------------------------------------------------------------------
void C_TFFlyingMob::FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options )
{
}


//-----------------------------------------------------------------------------
QAngle const &C_TFFlyingMob::GetRenderAngles( void )
{
	return m_myAngles;
}
