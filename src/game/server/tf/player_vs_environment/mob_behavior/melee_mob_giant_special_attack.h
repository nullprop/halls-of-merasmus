//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MELEE_MOB_GIANT_SPECIAL_ATTACK_H
#define MELEE_MOB_GIANT_SPECIAL_ATTACK_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CTFMeleeMobGiantSpecialAttack : public Action< CTFMeleeMob >
{
public:
	virtual ActionResult< CTFMeleeMob >	OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction );
	virtual ActionResult< CTFMeleeMob >	Update( CTFMeleeMob *me, float interval );

	virtual const char *GetName( void ) const	{ return "Giant Special Attack"; }		// return name of this action
private:

	void DoSpecialAttack( CTFMeleeMob *me );

	CountdownTimer m_attackTimer;
};

#endif // MELEE_MOB_GIANT_SPECIAL_ATTACK_H
