//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_emerge.cpp
// The Halloween Boss emerging from the ground
// Michael Booth, October 2011

#include "cbase.h"
#include "particle_parse.h"

#include "../tf_flying_mob.h"
#include "flying_mob_spawn.h"
#include "flying_mob_attack.h"


//---------------------------------------------------------------------------------------------
ActionResult< CTFFlyingMob > CTFFlyingMobSpawn::OnStart( CTFFlyingMob *me, Action< CTFFlyingMob > *priorAction )
{
	// just do teleport in code
	me->RemoveEffects( EF_NOINTERP | EF_NODRAW );

	DispatchParticleEffect( "eyeboss_tp_normal", me->GetAbsOrigin(), me->GetAbsAngles() );

	int animSequence = me->LookupSequence( "teleport_in" );
	if ( animSequence )
	{
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();
	}

	return Continue();
}


//----------------------------------------------------------------------------------
ActionResult< CTFFlyingMob > CTFFlyingMobSpawn::Update( CTFFlyingMob *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CTFFlyingMobAttack, "Pew pew" );
	}

	return Continue();
}
