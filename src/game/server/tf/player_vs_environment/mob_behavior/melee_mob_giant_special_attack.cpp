//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_fx.h"

#include "../tf_melee_mob.h"
#include "melee_mob_giant_special_attack.h"

ActionResult< CTFMeleeMob >	CTFMeleeMobGiantSpecialAttack::OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_SPECIAL_ATTACK1 );

	m_attackTimer.Start( 1 );

	return Continue();
}

ActionResult< CTFMeleeMob >	CTFMeleeMobGiantSpecialAttack::Update( CTFMeleeMob *me, float interval )
{
	if ( m_attackTimer.HasStarted() )
	{
		if ( m_attackTimer.IsElapsed() )
		{
			DoSpecialAttack( me );
			m_attackTimer.Invalidate();
		}
	}
	else if ( !me->IsValidSequence( me->GetSequence() ) || me->IsActivityFinished() )
	{
		return Done();
	}

	return Continue();
}

void CTFMeleeMobGiantSpecialAttack::DoSpecialAttack( CTFMeleeMob *me )
{
	CPVSFilter filter( me->GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0.0, "bomibomicon_ring", me->GetAbsOrigin(), vec3_angle );

	int nTargetTeam = TEAM_ANY;
	if ( me->GetTeamNumber() != TF_TEAM_HALLOWEEN )
	{
		nTargetTeam = me->GetTeamNumber() == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED;
	}

	CUtlVector< CTFPlayer* > pushedPlayers;
	TFGameRules()->PushAllPlayersAway(
		me->GetAbsOrigin(),
		me->GetSpecialAttackRange() * 1.5f,
		500.f,
		nTargetTeam,
		&pushedPlayers
	);

	CBaseEntity *pAttacker = me->GetOwnerEntity() ? me->GetOwnerEntity() : me;
	for ( int i=0; i < pushedPlayers.Count(); ++i )
	{
		Vector toVictim = pushedPlayers[i]->WorldSpaceCenter() - me->WorldSpaceCenter();
		toVictim.NormalizeInPlace();

		// hit!
		CTakeDamageInfo info( pAttacker, pAttacker, me->GetSpecialAttackDamage(), DMG_BLAST );
		info.SetDamageCustom( TF_DMG_CUSTOM_SPELL_SKELETON );
		CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 5.0f );
		pushedPlayers[i]->TakeDamage( info );
	}
}
