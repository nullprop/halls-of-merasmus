//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_approach_target.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"

#include "../tf_flying_mob.h"
#include "flying_mob_attack.h"
#include "tf_projectile_rocket.h"

#define FLYING_MOB_CHASE_MIN_DURATION 3.0f
#define FLYING_MOB_PROJECTILE_MODEL "models/props_halloween/eyeball_projectile.mdl"

extern ConVar tf_flying_mob_speed;

ConVar tf_flying_mob_projectile_speed( "tf_flying_mob_projectile_speed", "600", FCVAR_CHEAT );
ConVar tf_flying_mob_attack_interval_min( "tf_flying_mob_attack_interval_min", "1.2", FCVAR_CHEAT );
ConVar tf_flying_mob_attack_interval_max( "tf_flying_mob_attack_interval_max", "1.6", FCVAR_CHEAT );

//---------------------------------------------------------------------------------------------
ActionResult< CTFFlyingMob > CTFFlyingMobAttack::OnStart( CTFFlyingMob *me, Action< CTFFlyingMob > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFFlyingMob > CTFFlyingMobAttack::Update( CTFFlyingMob *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done();
	}

	SelectVictim( me );

	if ( m_attackTarget == NULL || !m_attackTarget->IsAlive() )
	{
		return Continue();
	}

	bool isVictimVisible = me->IsLineOfSightClear( m_attackTarget, CBaseCombatCharacter::IGNORE_ACTORS );

	if ( !isVictimVisible )
	{
		if ( !m_lingerTimer.IsElapsed() )
		{
			// wait a bit to see if we catch a glimpse of our victim again
			return Continue();
		}
	}

	m_lingerTimer.Start( 1.0f );

	Vector targetPosition = m_attackTarget->WorldSpaceCenter();

	// Shoot a rocket?
	if ( m_attackTimer.IsElapsed() && me->IsRangeLessThan( m_attackTarget, me->GetAttackRange() ) )
	{
		m_attackTimer.Start( RandomFloat( 
			tf_flying_mob_attack_interval_min.GetFloat(),
			tf_flying_mob_attack_interval_max.GetFloat()
		) );

		int animSequence = me->LookupSequence( "firing1" );
		if ( animSequence )
		{
			me->SetSequence( animSequence );
			me->SetPlaybackRate( 1.0f );
			me->SetCycle( 0 );
			me->ResetSequenceInfo();
		}

		float rocketSpeed = tf_flying_mob_projectile_speed.GetFloat();

		// lead our target
		float rangeBetween = me->GetRangeTo( targetPosition );

		const float veryCloseRange = 150.0f;
		if ( rangeBetween > veryCloseRange )
		{
			float timeToTravel = rangeBetween / rocketSpeed;

			Vector leadOffset = timeToTravel * m_attackTarget->GetAbsVelocity();

			CTraceFilterNoNPCsOrPlayer filter( me, COLLISION_GROUP_NONE );
			trace_t result;

			UTIL_TraceLine( me->WorldSpaceCenter(), m_attackTarget->GetAbsOrigin() + leadOffset, MASK_SOLID_BRUSHONLY, &filter, &result );

			if ( result.DidHit() )
			{
				const float errorTolerance = 300.0f;
				if ( ( result.endpos - targetPosition ).IsLengthGreaterThan( errorTolerance ) )
				{
					// rocket is going to hit an obstruction not near our victim - just aim right for them and hope for splash
					leadOffset = vec3_origin;
				}
			}

			targetPosition += leadOffset;
		}

		QAngle launchAngles;
		Vector toTarget = targetPosition - me->WorldSpaceCenter();
		VectorAngles( toTarget, launchAngles );

		CTFProjectile_Rocket *pRocket = CTFProjectile_Rocket::Create( me, me->WorldSpaceCenter(), launchAngles, me, me );
		if ( pRocket )
		{
			pRocket->SetModel( FLYING_MOB_PROJECTILE_MODEL );

			Vector forward;
			AngleVectors( launchAngles, &forward, NULL, NULL );

			Vector vecVelocity = forward * rocketSpeed;
			pRocket->SetAbsVelocity( vecVelocity );	
			pRocket->SetupInitialTransmittedGrenadeVelocity( vecVelocity );

			pRocket->EmitSound( "Weapon_RPG.Single" );
			pRocket->SetDamage( me->GetAttackDamage() );
			pRocket->SetCritical( false );
			pRocket->SetEyeBallRocket( true );
			pRocket->SetSpell( false );
			pRocket->ChangeTeam( me->GetTeamNumber() );
		}
	}

	// approach victim
	me->GetBodyInterface()->AimHeadTowards( targetPosition );
	me->GetLocomotionInterface()->SetDesiredSpeed( tf_flying_mob_speed.GetFloat() );
	me->GetLocomotionInterface()->Approach( targetPosition );

	return Continue();
}

void CTFFlyingMobAttack::SelectVictim( CTFFlyingMob *me )
{
	if ( IsPotentiallyChaseable( me, m_attackTarget ) && !m_attackTargetFocusTimer.IsElapsed() )
	{
		// Continue chasing current target
		return;
	}

	// pick a new victim to chase
	CBaseCombatCharacter *newVictim = NULL;
	CUtlVector< CTFPlayer * > playerVector;

	// collect everyone
	CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );

	float victimRangeSq = FLT_MAX;
	// find closest player
	for( int i=0; i<playerVector.Count(); ++i )
	{
		CTFPlayer *pPlayer = playerVector[i];
		if ( !IsPotentiallyChaseable( me, pPlayer ) )
		{
			continue;
		}

		// ignore stealth player
		if ( pPlayer->m_Shared.IsStealthed() )
		{
			if ( !pPlayer->m_Shared.InCond( TF_COND_BURNING ) &&
				!pPlayer->m_Shared.InCond( TF_COND_URINE ) &&
				!pPlayer->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) &&
				!pPlayer->m_Shared.InCond( TF_COND_BLEEDING ) )
			{
				// cloaked spies are invisible to us
				continue;
			}
		}

		// ignore ghost players
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		{
			continue;
		}

		float rangeSq = me->GetRangeSquaredTo( pPlayer );
		if ( rangeSq < victimRangeSq )
		{
			newVictim = pPlayer;
			victimRangeSq = rangeSq;
		}
	}

	if ( newVictim )
	{
		// we have a new victim
		m_attackTargetFocusTimer.Start( FLYING_MOB_CHASE_MIN_DURATION );
	}

	m_attackTarget = newVictim;
}

bool CTFFlyingMobAttack::IsPotentiallyChaseable( CTFFlyingMob *me, CBaseCombatCharacter *victim )
{
	if ( !victim )
	{
		return false;
	}

	if ( !victim->IsAlive() )
	{
		// victim is dead - pick a new one
		return false;
	}

	CTFNavArea *victimArea = (CTFNavArea *)victim->GetLastKnownArea();
	if ( !victimArea || victimArea->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE | TF_NAV_SPAWN_ROOM_RED ) )
	{
		// unreachable - pick a new victim
		return false;
	}

	return true;
}

EventDesiredResult< CTFFlyingMob > CTFFlyingMobAttack::OnStuck( CTFFlyingMob *me )
{
	// if we're stuck just die
	CTakeDamageInfo info( me, me, 99999.9f, DMG_SLASH );
	me->TakeDamage( info );

	return TryContinue( RESULT_TRY );
}