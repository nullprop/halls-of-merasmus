//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_emerge.h
// The Halloween Boss emerging from the ground
// Michael Booth, October 2011

#ifndef EYEBALL_BOSS_EMERGE_H
#define EYEBALL_BOSS_EMERGE_H

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
class CTFFlyingMobSpawn : public Action< CTFFlyingMob >
{
public:
	virtual ActionResult< CTFFlyingMob >	OnStart( CTFFlyingMob *me, Action< CTFFlyingMob > *priorAction );
	virtual ActionResult< CTFFlyingMob >	Update( CTFFlyingMob *me, float interval );
	virtual const char *GetName( void ) const	{ return "Flying mob spawn"; }		// return name of this action
};


#endif // EYEBALL_BOSS_EMERGE_H
