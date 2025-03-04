#include "cbase.h"
#include "tf_mob_projectile_fireball.h"
#include "tf_gamerules.h"

// Client specific.
#ifdef CLIENT_DLL
	#include "IEffects.h"
	#include "iclientmode.h"
	#include "dlight.h"
// Server specific.
#else
	#include "ilagcompensationmanager.h"
	#include "collisionutils.h"
	#include "particle_parse.h"
	#include "tf_projectile_base.h"
	#include "tf_gamerules.h"
    #include "tf_fx.h"
	#include "takedamageinfo.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( TFMobProjectile_Fireball, DT_TFMobProjectile_Fireball )
	BEGIN_NETWORK_TABLE( CTFMobProjectile_Fireball, DT_TFMobProjectile_Fireball )
	END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( tf_mob_projectile_fireball, CTFMobProjectile_Fireball );
PRECACHE_WEAPON_REGISTER( tf_mob_projectile_fireball);

#ifdef GAME_DLL
void CTFMobProjectile_Fireball::Spawn()
{
    SetModelScale( GetFireballScale() );
    BaseClass::Spawn();
}

void CTFMobProjectile_Fireball::RocketTouch( CBaseEntity *pOther )
{
    Assert( pOther );
    if ( !pOther || !pOther->IsSolid() || pOther->IsSolidFlagSet( FSOLID_VOLUME_CONTENTS ) || pOther->IsFuncLOD() )
        return;

    if ( pOther == GetOwnerEntity() || pOther->GetParent() == GetOwnerEntity() )
        return;

    // Handle hitting skybox (disappear).
    const trace_t *pTrace = &CBaseEntity::GetTouchTrace();

    if( pTrace->surface.flags & SURF_SKY )
    {
        UTIL_Remove( this );
        return;
    }

    // pass through ladders
    if( pTrace->surface.flags & CONTENTS_LADDER )
        return;

    if ( !ShouldTouchNonWorldSolid( pOther, pTrace ) )
        return;

    Explode( pTrace );

    UTIL_Remove( this );
}

void CTFMobProjectile_Fireball::Explode( const trace_t *pTrace )
{
    SetModelName( NULL_STRING );//invisible
    AddSolidFlags( FSOLID_NOT_SOLID );

    m_takedamage = DAMAGE_NO;

    // Pull out of the wall a bit.
    if ( pTrace->fraction != 1.0 )
    {
        SetAbsOrigin( pTrace->endpos + ( pTrace->plane.normal * 1.0f ) );
    }

    CBaseEntity *pOwner = GetOwnerEntity();
    const Vector &vecOrigin = GetAbsOrigin();

    // Particle
    if ( GetExplodeEffectParticle() )
    {	
        CPVSFilter filter( vecOrigin );
        TE_TFParticleEffect( filter, 0.0, GetExplodeEffectParticle(), vecOrigin, vec3_angle );
    }

    // Sounds
    if ( GetExplodeEffectSound() )
    {
        EmitSound( GetExplodeEffectSound() );
    }

    // Treat this trace exactly like radius damage
    CTraceFilterIgnorePlayers traceFilter( pOwner, COLLISION_GROUP_PROJECTILE );

    // Splash on everyone nearby.
    CBaseEntity *pListOfEntities[MAX_PLAYERS_ARRAY_SAFE];
    int iEntities = UTIL_EntitiesInSphere( pListOfEntities, ARRAYSIZE( pListOfEntities ), vecOrigin, GetDamageRadius(), FL_CLIENT | FL_FAKECLIENT | FL_NPC );
    for ( int i = 0; i < iEntities; ++i )
    {
        CBaseCombatCharacter *pBasePlayer = NULL;
        CTFPlayer *pPlayer = ToTFPlayer( pListOfEntities[i] );
        if ( !pPlayer )
        {
            pBasePlayer = dynamic_cast<CBaseCombatCharacter*>( pListOfEntities[i] );
        }
        else
        {
            pBasePlayer = pPlayer;
        }

        if ( !pBasePlayer || !pPlayer || !pPlayer->IsAlive() )
            continue;

        // Do a quick trace to see if there's any geometry in the way.
        trace_t trace;
        UTIL_TraceLine( vecOrigin, pPlayer->WorldSpaceCenter(), ( MASK_SHOT & ~( CONTENTS_HITBOX ) ), &traceFilter, &trace );
        //debugoverlay->AddLineOverlay( vecOrigin, pPlayer->WorldSpaceCenter(), 255, 0, 0, false, 10 );
        if ( trace.DidHitWorld() )
            continue;

        // Effects on the individual players
        ExplodeEffectOnTarget( pOwner, pPlayer, pBasePlayer );
    }

    CTakeDamageInfo info;
    info.SetAttacker( pOwner );
    info.SetInflictor( this ); 
    info.SetWeapon( GetLauncher() );
    info.SetDamage( m_flDamage * 0.1f );
    info.SetDamageCustom( GetCustomDamageType() );
    info.SetDamagePosition( vecOrigin );
    info.SetDamageType( DMG_BLAST );

    CTFRadiusDamageInfo radiusinfo( &info, vecOrigin, GetDamageRadius(), pOwner );
    TFGameRules()->RadiusDamage( radiusinfo );

    // Grenade remove
    //SetContextThink( &CBaseGrenade::SUB_Remove, gpGlobals->curtime, "RemoveThink" );

    // Remove the rocket.
    UTIL_Remove( this );
    
    SetTouch( NULL );
    AddEffects( EF_NODRAW );
    SetAbsVelocity( vec3_origin );
}

void CTFMobProjectile_Fireball::ExplodeEffectOnTarget( CBaseEntity *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget )
{
    if ( pBaseTarget->GetTeamNumber() == GetTeamNumber() )
        return;

    if ( pTarget )
    {
        if ( pTarget->m_Shared.IsInvulnerable() )
            return;

        if ( pTarget->m_Shared.InCond( TF_COND_PHASE ) )
            return;

        pTarget->m_Shared.SelfBurn( m_flDamage * 0.25f );
    }

    const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
    trace_t *pNewTrace = const_cast<trace_t*>( pTrace );

    CBaseEntity *pInflictor = GetLauncher();
    CTakeDamageInfo info;
    info.SetAttacker( pThrower );
    info.SetInflictor( this ); 
    info.SetWeapon( pInflictor );
    info.SetDamage( m_flDamage );
    info.SetDamageCustom( GetCustomDamageType() );
    info.SetDamagePosition( GetAbsOrigin() );
    info.SetDamageType( DMG_BURN );

    // Hurt 'em.
    Vector dir;
    AngleVectors( GetAbsAngles(), &dir );
    pBaseTarget->DispatchTraceAttack( info, dir, pNewTrace );
    ApplyMultiDamage();
}
#endif // GAME_DLL