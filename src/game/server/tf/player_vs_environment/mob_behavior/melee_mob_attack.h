//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MELEE_MOB_ATTACK_H
#define MELEE_MOB_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CTFMeleeMobAttack : public Action< CTFMeleeMob >
{
public:
	virtual ActionResult< CTFMeleeMob >	OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction );
	virtual ActionResult< CTFMeleeMob >	Update( CTFMeleeMob *me, float interval );

	virtual EventDesiredResult< CTFMeleeMob > OnStuck( CTFMeleeMob *me );
	virtual EventDesiredResult< CTFMeleeMob > OnContact( CTFMeleeMob *me, CBaseEntity *other, CGameTrace *result = NULL );
	virtual EventDesiredResult< CTFMeleeMob > OnOtherKilled( CTFMeleeMob *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info );

	virtual const char *GetName( void ) const	{ return "Attack"; }		// return name of this action

	CHandle<CBaseCombatCharacter> GetTarget( void ) const { return m_attackTarget; }

private:
	PathFollower m_path;

	CHandle< CBaseCombatCharacter > m_attackTarget;
	CountdownTimer m_attackTimer;
	CountdownTimer m_specialAttackTimer;
	CountdownTimer m_attackTargetFocusTimer;
	CountdownTimer m_tauntTimer;

	bool IsPotentiallyChaseable( CTFMeleeMob *me, CBaseCombatCharacter *victim );
	void SelectVictim( CTFMeleeMob *me );
};

#endif // MELEE_MOB_ATTACK_H
