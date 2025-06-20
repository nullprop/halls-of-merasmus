//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"

#include "../tf_melee_mob.h"
#include "melee_mob_attack.h"
#include "melee_mob_special_attack.h"
#include "melee_mob_giant_special_attack.h"

#define MELEE_MOB_CHASE_MIN_DURATION 3.0f

//----------------------------------------------------------------------------------
ActionResult< CTFMeleeMob >	CTFMeleeMobAttack::OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction )
{
	// smooth out the bot's path following by moving toward a point farther down the path
	m_path.SetMinLookAheadDistance( 100.0f );

	// start animation
	me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );

	m_specialAttackTimer.Start( RandomFloat( 5.f, 10.f ) );

	return Continue();
}


//----------------------------------------------------------------------------------
bool CTFMeleeMobAttack::IsPotentiallyChaseable( CTFMeleeMob *me, CBaseCombatCharacter *victim )
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

	if ( victim->GetGroundEntity() != NULL )
	{
		Vector victimAreaPos;
		victimArea->GetClosestPointOnArea( victim->GetAbsOrigin(), &victimAreaPos );
		if ( ( victim->GetAbsOrigin() - victimAreaPos ).AsVector2D().IsLengthGreaterThan( me->GetSpecialAttackRange() ) )
		{
			// off the mesh and unreachable - pick a new victim
			return false;
		}
	}

	return true;
}


//----------------------------------------------------------------------------------
void CTFMeleeMobAttack::SelectVictim( CTFMeleeMob *me )
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
	/*
	if ( me->GetTeamNumber() == TF_TEAM_RED )
	{
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS );
	}
	else if ( me->GetTeamNumber() == TF_TEAM_BLUE )
	{
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
	}
	else
	*/
	{
		CollectPlayers( &playerVector, TF_TEAM_RED, COLLECT_ONLY_LIVING_PLAYERS );
		CollectPlayers( &playerVector, TF_TEAM_BLUE, COLLECT_ONLY_LIVING_PLAYERS, APPEND_PLAYERS );
	}

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

		/*
		// ignore player who disguises as my team
		if ( pPlayer->m_Shared.InCond( TF_COND_DISGUISED ) && pPlayer->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() )
		{
			continue;
		}
		*/

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

	// find closest zombie
	/*
	for ( int i=0; i<ITFMeleeMobAutoList::AutoList().Count(); ++i )
	{
		CTFMeleeMob* pTFMeleeMob = static_cast< CTFMeleeMob* >( ITFMeleeMobAutoList::AutoList()[i] );
		if ( pTFMeleeMob->GetTeamNumber() == me->GetTeamNumber() )
		{
			continue;
		}

		if ( !IsPotentiallyChaseable( me, pTFMeleeMob ) )
		{
			continue;
		}

		float rangeSq = me->GetRangeSquaredTo( pTFMeleeMob );
		if ( rangeSq < victimRangeSq )
		{
			newVictim = pTFMeleeMob;
			victimRangeSq = rangeSq;
		}
	}
	*/

	if ( newVictim )
	{
		// we have a new victim
		m_attackTargetFocusTimer.Start( MELEE_MOB_CHASE_MIN_DURATION );
	}

	m_attackTarget = newVictim;
}


//----------------------------------------------------------------------------------
ActionResult< CTFMeleeMob >	CTFMeleeMobAttack::Update( CTFMeleeMob *me, float interval )
{
	if ( !me->IsAlive() )
	{
		return Done();
	}

	if ( !m_tauntTimer.IsElapsed() )
	{
		// wait for taunt to finish
		return Continue();
	}

	SelectVictim( me );

	if ( m_attackTarget == NULL || !m_attackTarget->IsAlive() )
	{
		return Continue();
	}

	// chase after our chase victim
	const float standAndSwingRange = me->GetAttackRange() * 0.9f;

	bool isLineOfSightClear = me->IsLineOfSightClear( m_attackTarget );

	if ( me->IsRangeGreaterThan( m_attackTarget, standAndSwingRange ) || !isLineOfSightClear )
	{
		if ( m_path.GetAge() > 0.5f )
		{
			CTFMeleeMobPathCost cost( me );
			m_path.Compute( me, m_attackTarget, cost );
		}

		m_path.Update( me );
	}

	const float specialAttackRange = me->GetSpecialAttackRange();
	const float meleeAttackRange = me->GetAttackRange();
	const float meleeAnimationRange = meleeAttackRange * 2.0f;

	const bool isInSpecialAttackRange = me->IsRangeLessThan( m_attackTarget, specialAttackRange );
	const bool isInMeleeAttackRange = me->IsRangeLessThan( m_attackTarget, meleeAttackRange );
	const bool isInMeleeAnimationRange = me->IsRangeLessThan( m_attackTarget, meleeAnimationRange );

	if ( isInSpecialAttackRange && ( !isInMeleeAnimationRange || me->GetMobType() == MobType_t::MELEE_GIANT ) )
	{
		if ( m_specialAttackTimer.IsElapsed() )
		{
			m_specialAttackTimer.Start( RandomFloat( 5.f, 10.f ) );
			switch ( me->GetMobType() )
			{
				case MobType_t::MELEE_NORMAL:
				default:
				{
					return SuspendFor( new CTFMeleeMobSpecialAttack, "Do Special Attack!" );
					break;
				}

				case MobType_t::MELEE_GIANT:
				{
					return SuspendFor( new CTFMeleeMobGiantSpecialAttack, "Do Giant Special Attack!" );
					break;
				}
			}
		}
	}

	// claw at attack target if they are close enough
	if ( isInMeleeAnimationRange )
	{
		me->GetLocomotionInterface()->FaceTowards( m_attackTarget->WorldSpaceCenter() );

		// swing!
		if ( !me->IsPlayingGesture( ACT_MP_ATTACK_STAND_MELEE ) )
		{
			me->AddGesture( ACT_MP_ATTACK_STAND_MELEE );
		}
	}

	if ( isInMeleeAttackRange )
	{
		if ( m_attackTimer.IsElapsed() )
		{
			m_attackTimer.Start( RandomFloat( 0.8f, 1.2f ) );

			Vector toVictim = m_attackTarget->WorldSpaceCenter() - me->WorldSpaceCenter();
			toVictim.NormalizeInPlace();

			// hit!
			CBaseEntity *pAttacker = me->GetOwnerEntity() ? me->GetOwnerEntity() : me;
			CTakeDamageInfo info( pAttacker, pAttacker, me->GetAttackDamage(), DMG_SLASH );
			info.SetDamageCustom( TF_DMG_CUSTOM_SPELL_SKELETON );
			CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 5.0f );
			m_attackTarget->TakeDamage( info );
		}
	}

	if ( !me->GetBodyInterface()->IsActivity( ACT_MP_RUN_MELEE ) )
	{
		me->GetBodyInterface()->StartActivity( ACT_MP_RUN_MELEE );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFMeleeMob > CTFMeleeMobAttack::OnStuck( CTFMeleeMob *me )
{
	// if we're stuck just die
	CTakeDamageInfo info( me, me, 99999.9f, DMG_SLASH );
	me->TakeDamage( info );

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFMeleeMob > CTFMeleeMobAttack::OnContact( CTFMeleeMob *me, CBaseEntity *other, CGameTrace *result )
{
	if ( other->IsPlayer() )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( other );
		if ( pTFPlayer )
		{
			if ( pTFPlayer->IsAlive() && me->GetTeamNumber() != TF_TEAM_HALLOWEEN && me->GetTeamNumber() != pTFPlayer->GetTeamNumber() )
			{
				// force attack the thing we bumped into
				// this prevents us from being stuck on dispensers, for example
				m_attackTarget = pTFPlayer;
				m_attackTargetFocusTimer.Start( MELEE_MOB_CHASE_MIN_DURATION );
			}
		}
	}

	return TryContinue( RESULT_TRY );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFMeleeMob > CTFMeleeMobAttack::OnOtherKilled( CTFMeleeMob *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	/*if ( victim && victim->IsPlayer() && me->GetLocomotionInterface()->IsOnGround() )
	{
		me->AddGestureSequence( me->LookupSequence( "taunt06" ) );
		m_tauntTimer.Start( 3.0f );
	}*/

	return TryContinue( RESULT_TRY );
}
