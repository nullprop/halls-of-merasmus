//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_approach_target.h
// The 2011 Halloween Boss
// Michael Booth, October 2011

#ifndef FLYING_MOB_ATTACK_H
#define FLYING_MOB_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CTFFlyingMobAttack : public Action< CTFFlyingMob >
{
public:
	virtual ActionResult< CTFFlyingMob >	OnStart( CTFFlyingMob *me, Action< CTFFlyingMob > *priorAction );
	virtual ActionResult< CTFFlyingMob >	Update( CTFFlyingMob *me, float interval );
	virtual EventDesiredResult< CTFFlyingMob > OnStuck( CTFFlyingMob *me );

	virtual const char *GetName( void ) const	{ return "Attack"; }		// return name of this action

private:
	bool IsPotentiallyChaseable( CTFFlyingMob *me, CBaseCombatCharacter *victim );
	void SelectVictim( CTFFlyingMob *me );

	CHandle< CBaseCombatCharacter > m_attackTarget;
	CountdownTimer m_attackTimer;
	CountdownTimer m_lingerTimer;
	CountdownTimer m_attackTargetFocusTimer;
};

#endif // FLYING_MOB_ATTACK_H
