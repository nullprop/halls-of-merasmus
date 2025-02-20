//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_shareddefs.h"
#include "../tf_melee_mob.h"
#include "melee_mob_attack.h"
#include "melee_mob_spawn.h"

ActionResult< CTFMeleeMob >	CTFMeleeMobSpawn::OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_TRANSITION );

	return Continue();
}


ActionResult< CTFMeleeMob >	CTFMeleeMobSpawn::Update( CTFMeleeMob *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CTFMeleeMobAttack, "Start Attack!" );
	}

	return Continue();
}