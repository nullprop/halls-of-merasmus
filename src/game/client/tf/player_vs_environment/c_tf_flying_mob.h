//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_TF_FLYING_MOB_H
#define C_TF_FLYING_MOB_H

#include "c_ai_basenpc.h"
#include "player_vs_environment/tf_mob_common.h"

//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the Halloween Eyeball Boss
 */
class C_TFFlyingMob : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_TFFlyingMob, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_TFFlyingMob();
	virtual ~C_TFFlyingMob();

	virtual void Spawn( void );
	virtual void OnDataChanged( DataUpdateType_t updateType );
	virtual bool IsNextBot() { return true; }

	virtual void FireEvent( const Vector& origin, const QAngle& angles, int event, const char *options );
	virtual void ClientThink();
	virtual void SetDormant( bool bDormant );

	virtual QAngle const &GetRenderAngles( void );

private:
	C_TFFlyingMob( const C_TFFlyingMob & );				// not defined, not accessible

	Vector m_lookAtSpot;
	int m_attitude;
	int m_priorAttitude;

	QAngle m_myAngles;

	int m_leftRightPoseParameter;
	int m_upDownPoseParameter;

	HPARTICLEFFECT		m_ghostEffect;

	HPARTICLEFFECT		m_auraEffect;

	CMaterialReference	m_InvulnerableMaterial;
};

#endif // C_TF_FLYING_MOB_H
