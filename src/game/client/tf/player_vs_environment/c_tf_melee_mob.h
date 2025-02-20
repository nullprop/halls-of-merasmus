//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_TF_MELEE_MOB_H
#define C_TF_MELEE_MOB_H

#include "c_ai_basenpc.h"
#include "props_shared.h"

//--------------------------------------------------------------------------------------------------------
/**
 * The client-side implementation of the Halloween TFMeleeMob
 */
class C_TFMeleeMob : public C_NextBotCombatCharacter
{
public:
	DECLARE_CLASS( C_TFMeleeMob, C_NextBotCombatCharacter );
	DECLARE_CLIENTCLASS();

	C_TFMeleeMob();

	virtual bool IsNextBot() { return true; }

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const;

	virtual void BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed ) OVERRIDE;

private:
	C_TFMeleeMob( const C_TFMeleeMob & );				// not defined, not accessible

	float m_flHeadScale;
};

#endif // C_TF_MELEE_MOB_H
