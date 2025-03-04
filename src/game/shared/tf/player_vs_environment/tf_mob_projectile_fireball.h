#ifndef TF_MOB_PROJECTILE_FIREBALL_H
#define TF_MOB_PROJECTILE_FIREBALL_H

#ifdef CLIENT_DLL
	#include "c_tf_projectile_rocket.h"
	#include "c_tf_player.h"

	#define CTFMobProjectile_Fireball C_TFMobProjectile_Fireball
#else
	#include "tf_projectile_rocket.h"
	#include "tf_player.h"
#endif

class CTFMobProjectile_Fireball : public CTFProjectile_Rocket
{
public:
	DECLARE_CLASS( CTFMobProjectile_Fireball, CTFProjectile_Rocket );
	DECLARE_NETWORKCLASS();

	CTFMobProjectile_Fireball()
	{
#ifdef GAME_DLL
		m_pszExplodeParticleName = "bombinomicon_burningdebris"; // "spell_fireball_tendril_parent_red"
#endif // GAME_DLL
	}

	virtual int			GetWeaponID( void ) const			{ return TF_WEAPON_SPELLBOOK_PROJECTILE; }
	virtual float		GetDamageRadius() const				{ return 200.0f; }
	virtual int			GetCustomDamageType() const OVERRIDE	{ return TF_DMG_CUSTOM_SPELL_FIREBALL; }
	virtual bool		IsDeflectable() OVERRIDE { return false; }

#ifdef GAME_DLL
	virtual void Spawn() OVERRIDE;
	virtual int UpdateTransmitState() OVERRIDE { return SetTransmitState( FL_EDICT_PVSCHECK ); }

	virtual void RocketTouch( CBaseEntity *pOther ) OVERRIDE;

	virtual void Explode( const trace_t *pTrace );

	virtual const char *GetProjectileModelName( void ) { return ""; } // We dont have a model by default, and that's OK

	virtual void		ExplodeEffectOnTarget( CBaseEntity *pThrower, CTFPlayer *pTarget, CBaseCombatCharacter *pBaseTarget );

	virtual const char *GetExplodeEffectParticle() const	{ return m_pszExplodeParticleName; }
	void SetExplodeParticleName( const char *pszName )		{ m_pszExplodeParticleName = pszName; }
	virtual const char *GetExplodeEffectSound()	const		{ return "Halloween.spell_fireball_impact"; }
#endif

#ifdef CLIENT_DLL
	virtual const char *GetTrailParticleName( void )
	{ 
		return GetTeamNumber() == TF_TEAM_BLUE ? "spell_fireball_small_blue" : "spell_fireball_small_red";
	}
#endif

protected:
	virtual float GetFireballScale() const { return 0.01f; }

#ifdef GAME_DLL
private:
	const char *m_pszExplodeParticleName;
#endif // GAME_DLL
};

#endif // TF_MOB_PROJECTILE_FIREBALL_H