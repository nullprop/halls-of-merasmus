#include "hom_upgrades.h"
#include "tf_player.h"
#include "NextBot/NextBot.h"
#include "tf_shareddefs.h"
#include "tf_fx.h"
#include "tf_gamerules.h"

#define HOM_MAX_EXPLOSION_EFFECTS 32
#define HOM_EXPLOSION_DAMAGE 50.0f
#define HOM_EXPLOSION_RADIUS 100.0f

void GiveUpgrade( const CCommand &args )
{
	CTFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( 1 ) );
	
	if ( !pPlayer )
		return;

	HomUpgrade_t upgradeType = HomUpgrade_t( ( args.ArgC() > 1 ) ? atoi(args[1]) : 0 );

	CHomUpgradeHandler::ApplyUpgrade( pPlayer, upgradeType );
}

ConCommand cc_give_upgrade( "give_upgrade", GiveUpgrade, 0, FCVAR_CHEAT );

void CHomUpgradeHandler::ApplyUpgrade( CTFPlayer *pPlayer, HomUpgrade_t upgrade )
{
    pPlayer->m_upgrades[upgrade]++;
    pPlayer->m_iUpgradeCount++;

    if ( upgrade == HomUpgrade_t::HEALTH_10 )
    {
        float maxHealth = pPlayer->GetMaxHealth();
        maxHealth += maxHealth * 0.10f;
        pPlayer->SetMaxHealth( maxHealth );
        pPlayer->SetHealth( maxHealth );
    }
}

CTFPlayer* GetAttackingPlayer( const CTakeDamageInfo &info )
{
    CBaseEntity *pAttacker = info.GetAttacker();
	CTFPlayer *pTFAttacker = ToTFPlayer( pAttacker );
    if ( pTFAttacker )
        return pTFAttacker;

    // Try the owner if projectile
    CBaseProjectile *pProjectile = dynamic_cast< CBaseProjectile* >( pAttacker );
    if ( pProjectile )
    {
        pTFAttacker = ToTFPlayer( pProjectile->GetOwnerEntity() );
        if ( pTFAttacker )
            return pTFAttacker;
    }

    // Try the inflictor instead of attacker
    pAttacker = info.GetInflictor();
    pTFAttacker = ToTFPlayer( pAttacker );
    if ( pTFAttacker )
        return pTFAttacker;

    pProjectile = dynamic_cast< CBaseProjectile* >( pAttacker );
    if ( pProjectile )
    {
        pTFAttacker = ToTFPlayer( pProjectile->GetOwnerEntity() );
        if ( pTFAttacker )
            return pTFAttacker;
    }

    return NULL;
}

float CHomUpgradeHandler::OnTakeDamage_Alive( float flDamage, const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity )
{
    CTFPlayer *pTFAttacker = GetAttackingPlayer( info );

    if ( pTFAttacker )
    {
        flDamage = OnDealDamage_Alive_Player( flDamage, info, pTFAttacker );
    }

	CTFPlayer *pTFVictim = ToTFPlayer( pVictimBaseEntity );
    if ( pTFVictim )
    {
        flDamage = OnTakeDamage_Alive_Player( flDamage, info, pTFVictim );
    }

    return flDamage;
}

void CHomUpgradeHandler::OnMobKilled( NextBotCombatCharacter *pMob, const CTakeDamageInfo &info )
{
    CTFPlayer *pTFAttacker = GetAttackingPlayer( info );

    if ( pTFAttacker )
    {
        // Explode enemies on kill?
        const QAngle vecAngles = pMob->GetAbsAngles();
        const Vector vecOrigin = pMob->GetAbsOrigin();
        // Spread multiple explosions around a bit
        const float spread = 8.0f * pTFAttacker->m_upgrades[HomUpgrade_t::EXPLODE_ON_KILL];
        for ( int i = 0; i < pTFAttacker->m_upgrades[HomUpgrade_t::EXPLODE_ON_KILL]; i++ )
        {
            Vector vecExplosion = vecOrigin + Vector(
                RandomFloat( -48.0f - spread, 48.0f + spread ),
                RandomFloat( -48.0f - spread, 48.0f + spread ),
                RandomFloat( 16.0f, 64.0f + 0.5f * spread )
            );

            // Limit number of visuals if crazy amount of upgrades
            if ( i < HOM_MAX_EXPLOSION_EFFECTS )
            {
                // Add some delay to FX if multiple explosions
                const float flDelay = i > 0 ? RandomFloat( 0.05f, 0.30f ) : 0.0f;

                CPVSFilter filter( vecExplosion );
                TE_TFExplosion( filter, flDelay, vecExplosion, Vector( 0, 0, 1 ), TF_WEAPON_PUMPKIN_BOMB, -1 );
                TE_TFParticleEffect( filter, flDelay, "pumpkin_explode", vecExplosion, vecAngles );

                UTIL_ScreenShake( vecExplosion, 15.0f, 5.0f, 1.0f, HOM_EXPLOSION_RADIUS * 5.0f, SHAKE_START, true );
            }

            int iDmgType = DMG_BLAST | DMG_USEDISTANCEMOD;
            CTakeDamageInfo info( pTFAttacker, pTFAttacker, NULL, vecExplosion, vecExplosion, HOM_EXPLOSION_DAMAGE, iDmgType, TF_DMG_CUSTOM_PUMPKIN_BOMB, &vecExplosion );
            info.SetForceFriendlyFire( true );

            CTFRadiusDamageInfo radiusinfo( &info, vecExplosion, HOM_EXPLOSION_RADIUS, NULL );
            TFGameRules()->RadiusDamage( radiusinfo );
        }
    }
}

float CHomUpgradeHandler::OnDealDamage_Alive_Player( float flDamage, const CTakeDamageInfo &info, CTFPlayer *pPlayer )
{
    for ( int i = 0; i < pPlayer->m_upgrades[HomUpgrade_t::DAMAGE_10]; i++ )
    {
        flDamage *= 1.10f;
    }

    return flDamage;
}

float CHomUpgradeHandler::OnTakeDamage_Alive_Player( float flDamage, const CTakeDamageInfo &info, CTFPlayer *pPlayer )
{
    for ( int i = 0; i < pPlayer->m_upgrades[HomUpgrade_t::DAMAGE_RESIST_10]; i++ )
    {
        flDamage *= 0.9f;
    }

    return flDamage;
}
