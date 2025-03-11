//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_FLYING_MOB_BODY_H
#define TF_FLYING_MOB_BODY_H

#include "animation.h"
#include "bot_npc/bot_npc_body.h"

class INextBot;


class CTFFlyingMobBody : public CBotNPCBody
{
public:
	CTFFlyingMobBody( INextBot *bot );
	virtual ~CTFFlyingMobBody() { }

	virtual void Update( void );

	virtual void AimHeadTowards( const Vector &lookAtPos, 
								 LookAtPriorityType priority = BORING, 
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = NULL,
								 const char *reason = NULL );		// aim the bot's head towards the given goal
	virtual void AimHeadTowards( CBaseEntity *subject,
								 LookAtPriorityType priority = BORING, 
								 float duration = 0.0f,
								 INextBotReply *replyWhenAimed = NULL,
								 const char *reason = NULL );		// continually aim the bot's head towards the given subject

	virtual float GetMaxHeadAngularVelocity( void ) const			// return max turn rate of head in degrees/second
	{
		return 3000.0f;
	}

private:
	int m_leftRightPoseParameter;
	int m_upDownPoseParameter;

	Vector m_lookAtSpot;
};


#endif // TF_FLYING_MOB_BODY_H
