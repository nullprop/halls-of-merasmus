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
#include "melee_mob_special_attack.h"
#include "melee_mob_attack.h"
#include "player_vs_environment/tf_mob_projectile_fireball.h"

ActionResult< CTFMeleeMob >	CTFMeleeMobSpecialAttack::OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction )
{
	// TODO
	//me->GetBodyInterface()->StartActivity( ACT_SPECIAL_ATTACK1 );

	m_attackTimer.Start( 1 );

	CTFMeleeMobAttack *priorAttack = dynamic_cast<CTFMeleeMobAttack*>(priorAction);
	if ( priorAttack )
	{
		m_attackTarget = priorAttack->GetTarget();
	}

	return Continue();
}

ActionResult< CTFMeleeMob >	CTFMeleeMobSpecialAttack::Update( CTFMeleeMob *me, float interval )
{
	if (m_attackTarget)
	{
		me->GetLocomotionInterface()->FaceTowards( m_attackTarget->WorldSpaceCenter() );
	}

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

void CTFMeleeMobSpecialAttack::DoSpecialAttack( CTFMeleeMob *me )
{
	const Vector origin = me->EyePosition();
	const QAngle angles = me->GetAbsAngles();

	Vector vecForward;

	if ( m_attackTarget )
	{
		vecForward = m_attackTarget->GetAbsOrigin() - origin + Vector(0, 0, 64);
		vecForward.NormalizeInPlace();
	}
	else
	{
		AngleVectors( angles, &vecForward );
		vecForward.z = 0.2f;
		vecForward.NormalizeInPlace();
	}

	const Vector velocity = vecForward * 800.0f;

	CTFMobProjectile_Fireball *pFireball = static_cast< CTFMobProjectile_Fireball* >( CBaseEntity::CreateNoSpawn( "tf_mob_projectile_fireball", origin, angles, me ) );
	if ( pFireball )
	{
		pFireball->SetOwnerEntity( me );
		pFireball->SetAbsVelocity( velocity );
		pFireball->SetDamage( me->GetSpecialAttackDamage() );
		pFireball->ChangeTeam( me->GetTeamNumber() );

		IPhysicsObject *pPhysicsObject = pFireball->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity( &velocity, NULL );
		}

		DispatchSpawn( pFireball );
	}
}
