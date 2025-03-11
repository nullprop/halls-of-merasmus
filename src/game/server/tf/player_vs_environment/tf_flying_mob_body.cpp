//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "NextBot.h"
#include "tf_flying_mob.h"
#include "tf_flying_mob_body.h"


CTFFlyingMobBody::CTFFlyingMobBody( INextBot *bot ) : CBotNPCBody( bot ) 
{
	m_leftRightPoseParameter = -1;
	m_upDownPoseParameter = -1;
	m_lookAtSpot = vec3_origin;
}


//---------------------------------------------------------------------------------------------
void CTFFlyingMobBody::Update( void )
{
	CBaseCombatCharacter *me = GetBot()->GetEntity();

	// track client-side rotation
	Vector myForward;
	me->GetVectors( &myForward, NULL, NULL );

	const float myApproachRate = 3.0f; // 1.0f;

	Vector toTarget = m_lookAtSpot - me->WorldSpaceCenter();
	toTarget.NormalizeInPlace();

	myForward += toTarget * myApproachRate * GetUpdateInterval();
	myForward.NormalizeInPlace();

	QAngle myNewAngles;
	VectorAngles( myForward, myNewAngles );

	me->SetAbsAngles( myNewAngles );

	// move the animation ahead in time	
	me->StudioFrameAdvance();
	me->DispatchAnimEvents( me );
}


//---------------------------------------------------------------------------------------------
// Aim the bot's head towards the given goal
void CTFFlyingMobBody::AimHeadTowards( const Vector &lookAtPos, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	CTFFlyingMob *me = (CTFFlyingMob *)GetBot()->GetEntity();

	m_lookAtSpot = lookAtPos;
	me->SetLookAtTarget( lookAtPos );
}


//---------------------------------------------------------------------------------------------
// Continually aim the bot's head towards the given subject
void CTFFlyingMobBody::AimHeadTowards( CBaseEntity *subject, LookAtPriorityType priority, float duration, INextBotReply *replyWhenAimed, const char *reason )
{
	CTFFlyingMob *me = (CTFFlyingMob *)GetBot()->GetEntity();

	me->SetLookAtTarget( subject->EyePosition() );

	if ( !subject )
		return;

	m_lookAtSpot = subject->EyePosition();
}