//========= Copyright Valve Corporation, All rights reserved. ============//
// c_eyeball_boss.cpp

#include "cbase.h"
#include "NextBot/C_NextBot.h"
#include "c_tf_melee_mob.h"
#include "tf_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
IMPLEMENT_CLIENTCLASS_DT( C_TFMeleeMob, DT_TFMeleeMob, CTFMeleeMob )
	RecvPropFloat( RECVINFO( m_flHeadScale ) ),
END_RECV_TABLE()


C_TFMeleeMob::C_TFMeleeMob()
{
}


bool C_TFMeleeMob::ShouldCollide( int collisionGroup, int contentsMask ) const
{
	if ( collisionGroup == COLLISION_GROUP_PLAYER_MOVEMENT )
		return false;

	return BaseClass::ShouldCollide( collisionGroup, contentsMask );
}

extern void BuildBigHeadTransformations( CBaseAnimating *pObject, CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed, float flScale );
void C_TFMeleeMob::BuildTransformations( CStudioHdr *hdr, Vector *pos, Quaternion q[], const matrix3x4_t& cameraTransform, int boneMask, CBoneBitList &boneComputed )
{
	BaseClass::BuildTransformations( hdr, pos, q, cameraTransform, boneMask, boneComputed );

	m_BoneAccessor.SetWritableBones( BONE_USED_BY_ANYTHING );
	BuildBigHeadTransformations( this, hdr, pos, q, cameraTransform, boneMask, boneComputed, m_flHeadScale );
}
