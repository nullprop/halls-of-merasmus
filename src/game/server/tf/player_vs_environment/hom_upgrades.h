#ifndef HOM_UPGRADE_H
#define HOM_UPGRADE_H

class CTFPlayer;
class NextBotCombatCharacter;

typedef enum {
    HEALTH_10,          // 10% extra health
    DAMAGE_10,          // 10% extra damage
    DAMAGE_RESIST_10,   // 10% damage resist
    EXPLODE_ON_KILL,    // enemies explode when killed
    UPGRADE_COUNT
} HomUpgrade_t;

class CHomUpgradeHandler
{
public:
    static void ApplyUpgrade( CTFPlayer *pPlayer, HomUpgrade_t upgrade );
    static float OnTakeDamage_Alive( float flDamage, const CTakeDamageInfo &info, CBaseEntity *pVictimBaseEntity );
    static void OnMobKilled( NextBotCombatCharacter *pMob, const CTakeDamageInfo &info );

private:
    static float OnDealDamage_Alive_Player( float flDamage, const CTakeDamageInfo &info, CTFPlayer *pPlayer );
    static float OnTakeDamage_Alive_Player( float flDamage, const CTakeDamageInfo &info, CTFPlayer *pPlayer );
};

#endif // HOM_UPGRADE_H