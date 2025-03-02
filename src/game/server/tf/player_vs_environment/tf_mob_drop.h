//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF MobDrop.
//
//=============================================================================//
#ifndef TF_MOB_DROP_H
#define TF_MOB_DROP_H

#ifdef _WIN32
#pragma once
#endif

#include "tf_powerup.h"
#include "player.h"
#include "tf_shareddefs.h"


//=============================================================================
//
// CTF MobDrop class.
//

extern ConVar tf_mob_drop_lifetime;

DECLARE_AUTO_LIST( ITFMobDropAutoList );

class CTFMobDrop : public CTFPowerup, public ITFMobDropAutoList
{
public:
	DECLARE_CLASS( CTFMobDrop, CTFPowerup );
	DECLARE_SERVERCLASS();

	CTFMobDrop();
	~CTFMobDrop();

	void	Spawn( void );
	void	Precache( void );
	void	UpdateOnRemove( void );
	virtual int UpdateTransmitState() OVERRIDE;
	virtual int ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;
	virtual float	GetLifeTime( void ) { return tf_mob_drop_lifetime.GetFloat(); }
	bool	MyTouch( CBasePlayer *pPlayer );
	virtual bool AffectedByRadiusCollection() const { return true; }

	void	SetAmount( float flAmount );
	void	SetClaimed( void )
	{
		m_bClaimed = true;
		SetFriction(0.2f); // Radius collection code "steers" packs toward the player
	}
	bool	IsClaimed( void ) { return m_bClaimed; }	// So don't allow other players to interfere
	
	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_CASH_LARGE; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_large.mdl"; }
	virtual const char *GetPickupSound( void ) { return "MVM.MoneyPickup"; }
	virtual const char *GetVanishSound( void ) { return "MVM.MoneyVanish"; }
	virtual const char *GetSpawnParticles( void ) { return "mvm_cash_embers"; }
	virtual const char *GetVanishParticles( void ) { return "mvm_cash_explosion"; }

protected:
	virtual void ComeToRest( void );

	void BlinkThink( void );

	int		m_nAmount;
	int m_blinkCount;
	CountdownTimer m_blinkTimer;
	bool	m_bTouched;
	bool	m_bClaimed;
	//CNetworkVar( bool, m_bDistributed );
};

// --------------------------------
// Cash
// --------------------------------
class CTFMobDropCashLarge : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropCashLarge, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_CASH_LARGE; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_large.mdl"; }
};

class CTFMobDropCashMedium : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropCashMedium, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_CASH_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_medium.mdl"; }
};

class CTFMobDropCashSmall : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropCashSmall, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_CASH_SMALL; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/currencypack_small.mdl"; }
};

// --------------------------------
// Ammo
// --------------------------------
class CTFMobDropAmmoLarge : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropAmmoLarge, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_AMMO_LARGE; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/ammopack_large.mdl"; }
	virtual const char *GetPickupSound( void ) { return "AmmoPack.Touch"; }
	virtual const char *GetVanishSound( void ) { return nullptr; }
	virtual const char *GetSpawnParticles( void ) { return nullptr; }
	virtual const char *GetVanishParticles( void ) { return nullptr; }
};

class CTFMobDropAmmoMedium : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropAmmoMedium, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_AMMO_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/ammopack_medium.mdl"; }
	virtual const char *GetPickupSound( void ) { return "AmmoPack.Touch"; }
	virtual const char *GetVanishSound( void ) { return nullptr; }
	virtual const char *GetSpawnParticles( void ) { return nullptr; }
	virtual const char *GetVanishParticles( void ) { return nullptr; }
};

class CTFMobDropAmmoSmall : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropAmmoSmall, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_AMMO_SMALL; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/ammopack_small.mdl"; }
	virtual const char *GetPickupSound( void ) { return "AmmoPack.Touch"; }
	virtual const char *GetVanishSound( void ) { return nullptr; }
	virtual const char *GetSpawnParticles( void ) { return nullptr; }
	virtual const char *GetVanishParticles( void ) { return nullptr; }
};

// --------------------------------
// Health
// --------------------------------
class CTFMobDropHealthLarge : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropHealthLarge, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_HEALTH_LARGE; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/medkit_large.mdl"; }
	virtual const char *GetPickupSound( void ) { return "HealthKit.Touch"; }
	virtual const char *GetVanishSound( void ) { return nullptr; }
	virtual const char *GetSpawnParticles( void ) { return nullptr; }
	virtual const char *GetVanishParticles( void ) { return nullptr; }
};

class CTFMobDropHealthMedium : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropHealthMedium, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_HEALTH_MEDIUM; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/medkit_medium.mdl"; }
	virtual const char *GetPickupSound( void ) { return "HealthKit.Touch"; }
	virtual const char *GetVanishSound( void ) { return nullptr; }
	virtual const char *GetSpawnParticles( void ) { return nullptr; }
	virtual const char *GetVanishParticles( void ) { return nullptr; }
};

class CTFMobDropHealthSmall : public CTFMobDrop
{
public:
	DECLARE_CLASS( CTFMobDropHealthSmall, CTFMobDrop );

	virtual MobDrops_t	GetDropType( void ) { return TF_MOB_DROP_HEALTH_SMALL; }
	virtual const char *GetDefaultPowerupModel( void ) { return "models/items/medkit_small.mdl"; }
	virtual const char *GetPickupSound( void ) { return "HealthKit.Touch"; }
	virtual const char *GetVanishSound( void ) { return nullptr; }
	virtual const char *GetSpawnParticles( void ) { return nullptr; }
	virtual const char *GetVanishParticles( void ) { return nullptr; }
};

#endif // TF_MOB_DROP_H


