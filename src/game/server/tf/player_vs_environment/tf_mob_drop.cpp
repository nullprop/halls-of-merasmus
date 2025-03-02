//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: CTF MobDrop.
//
//=============================================================================//
#include "cbase.h"
#include "items.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "tf_player.h"
#include "tf_team.h"
#include "engine/IEngineSound.h"
#include "tf_mob_drop.h"
#include "tf_gamestats.h"
#include "tf_mann_vs_machine_stats.h"
#include "world.h"
#include "particle_parse.h"
#include "player_vs_environment/tf_population_manager.h"
#include "collisionutils.h"
#include "tf_objective_resource.h"

//=============================================================================
//
// CTF MobDrop defines.
//

#define TF_MOBDROP_BLINK_PERIOD	5.0f		// how long pack blinks before it vanishes
#define TF_MOBDROP_BLINK_DURATION  0.25f	// how long a blink lasts

#define TF_MOBDROP_GLOW_THINK_TIME	0.1f	// how often should we check if should glow

ConVar tf_mob_drop_lifetime( "tf_mob_drop_lifetime", "30", FCVAR_CHEAT );

LINK_ENTITY_TO_CLASS( item_mobdrop_cash_large, 	  CTFMobDropCashLarge    );
LINK_ENTITY_TO_CLASS( item_mobdrop_cash_medium,   CTFMobDropCashMedium   );
LINK_ENTITY_TO_CLASS( item_mobdrop_cash_small, 	  CTFMobDropCashSmall    );
LINK_ENTITY_TO_CLASS( item_mobdrop_ammo_large, 	  CTFMobDropAmmoLarge    );
LINK_ENTITY_TO_CLASS( item_mobdrop_ammo_medium,   CTFMobDropAmmoMedium   );
LINK_ENTITY_TO_CLASS( item_mobdrop_ammo_small, 	  CTFMobDropAmmoSmall    );
LINK_ENTITY_TO_CLASS( item_mobdrop_health_large,  CTFMobDropHealthLarge  );
LINK_ENTITY_TO_CLASS( item_mobdrop_health_medium, CTFMobDropHealthMedium );
LINK_ENTITY_TO_CLASS( item_mobdrop_health_small,  CTFMobDropHealthSmall  );


IMPLEMENT_SERVERCLASS_ST( CTFMobDrop, DT_MobDrop )
//	SendPropBool( SENDINFO( m_bDistributed ) ),
END_SEND_TABLE()


IMPLEMENT_AUTO_LIST( ITFMobDropAutoList );


//=============================================================================
//
// CTF MobDrop functions.
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFMobDrop::CTFMobDrop()
{
	m_nAmount = 0;
	m_bTouched = false;
	m_bClaimed = false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFMobDrop::~CTFMobDrop()
{
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMobDrop::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();

	if ( !m_bTouched )
	{
		const char* particles = GetVanishParticles();
		if ( particles )
			DispatchParticleEffect( particles, GetAbsOrigin(), GetAbsAngles() );
		
		const char* sound = GetVanishSound();
		if (sound)
		{
			CReliableBroadcastRecipientFilter filter;
			EmitSound( filter, entindex(), GetVanishSound() );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CTFMobDrop::UpdateTransmitState( void )
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CTFMobDrop::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	return FL_EDICT_ALWAYS;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMobDrop::Spawn( void )
{
	BaseClass::Spawn();
	m_blinkCount = 0;
	m_blinkTimer.Invalidate();
	SetContextThink( &CTFMobDrop::BlinkThink, gpGlobals->curtime + GetLifeTime() - TF_MOBDROP_BLINK_PERIOD - RandomFloat( 0.0, TF_MOBDROP_BLINK_DURATION ), "MobDropWaitingToBlinkThink" );
	
	// Force collision size to see if this fixes a bunch of stuck-in-geo issues goes away
	SetCollisionBounds( Vector( -10, -10, -10 ), Vector( 10, 10, 10 ) );

	const char* particles = GetSpawnParticles();
	if ( particles )
		DispatchParticleEffect( particles, PATTACH_ABSORIGIN_FOLLOW, this );
}

//-----------------------------------------------------------------------------
// Purpose: Blink off/on when about to expire and play expire sound
//-----------------------------------------------------------------------------
void CTFMobDrop::BlinkThink( void )
{
	// This means the pack was claimed by a player via Radius Currency Collection and
	// is likely flying toward them.  Regardless, one second later it fires Touch().
	if ( IsClaimed() )
		return;

	++m_blinkCount;

	SetRenderMode( kRenderTransAlpha );

	if ( m_blinkCount & 0x1 )
	{
		SetRenderColorA( 25 );
	}
	else
	{
		SetRenderColorA( 255 );
	}

	SetContextThink( &CTFMobDrop::BlinkThink, gpGlobals->curtime + TF_MOBDROP_BLINK_DURATION, "MobDropBlinkThink" );
}


//-----------------------------------------------------------------------------
// Become touchable when we are at rest
//-----------------------------------------------------------------------------
void CTFMobDrop::ComeToRest( void )
{
	BaseClass::ComeToRest();

	if ( IsClaimed() )
		return;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the value of a custom pack
//-----------------------------------------------------------------------------
void CTFMobDrop::SetAmount( float nAmount )
{
	m_nAmount = nAmount;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFMobDrop::Precache( void )
{
	const char* pickupSound = GetPickupSound();
	const char* vanishSound = GetVanishSound();
	if ( pickupSound )
		PrecacheScriptSound( pickupSound );
	if ( vanishSound )
		PrecacheScriptSound( vanishSound );

	PrecacheModel( GetDefaultPowerupModel() );

	const char* spawnParticles = GetSpawnParticles();
	const char* vanishParticles = GetVanishParticles();
	if ( spawnParticles )
		PrecacheParticleSystem( spawnParticles );
	if ( vanishParticles )
		PrecacheParticleSystem( vanishParticles );
	
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CTFMobDrop::MyTouch( CBasePlayer *pPlayer )
{
	if ( ValidTouch( pPlayer ) && !m_bTouched )
	{
		CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );
		if ( !pTFPlayer )
			return false;

		if ( pTFPlayer->IsBot() )
			return false;

		const char* sound = GetPickupSound();
		if (sound)
		{
			CReliableBroadcastRecipientFilter filter;
			EmitSound( filter, entindex(), sound );
		}

		switch (GetDropType())
		{
			case TF_MOB_DROP_CASH_LARGE:
			case TF_MOB_DROP_CASH_MEDIUM:
			case TF_MOB_DROP_CASH_SMALL:
			{
				TFGameRules()->DistributeCurrencyAmount( m_nAmount, pTFPlayer );
				CTF_GameStats.Event_PlayerCollectedCurrency( pTFPlayer, m_nAmount );
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_MONEY_PICKUP );
				break;
			}

			case TF_MOB_DROP_AMMO_LARGE:
			case TF_MOB_DROP_AMMO_MEDIUM:
			case TF_MOB_DROP_AMMO_SMALL:
			{
				pTFPlayer->GiveAmmo( m_nAmount, TF_AMMO_PRIMARY, true, kAmmoSource_Pickup );
				pTFPlayer->GiveAmmo( m_nAmount, TF_AMMO_SECONDARY, true, kAmmoSource_Pickup );
				pTFPlayer->GiveAmmo( m_nAmount, TF_AMMO_METAL, true, kAmmoSource_Pickup );
				break;
			}

			case TF_MOB_DROP_HEALTH_LARGE:
			case TF_MOB_DROP_HEALTH_MEDIUM:
			case TF_MOB_DROP_HEALTH_SMALL:
			{
				float flHealth = m_nAmount;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( pTFPlayer, flHealth, mult_health_frompacks );
				int nHealthGiven = pTFPlayer->TakeHealth( flHealth, DMG_GENERIC );
				break;
			}

			default:
			{
				Assert(0);
				break;
			}
		}

		pTFPlayer->SetLastObjectiveTime( gpGlobals->curtime );
		
		m_bTouched = true;
	}

	return m_bTouched;
}

