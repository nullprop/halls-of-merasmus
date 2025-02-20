//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef MELEE_MOB_SPAWN_H
#define MELEE_MOB_SPAWN_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CTFMeleeMobSpawn : public Action< CTFMeleeMob >
{
public:
	virtual ActionResult< CTFMeleeMob >	OnStart( CTFMeleeMob *me, Action< CTFMeleeMob > *priorAction );
	virtual ActionResult< CTFMeleeMob >	Update( CTFMeleeMob *me, float interval );

	virtual const char *GetName( void ) const	{ return "Spawn"; }		// return name of this action

private:
};

#endif // MELEE_MOB_SPAWN_H
