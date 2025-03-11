// tf_mob_generator.h
// Entity to spawn a collection of Mobs

#ifndef TF_MOB_GENERATOR_H
#define TF_MOB_GENERATOR_H

#include "player_vs_environment/tf_melee_mob.h"
#include "player_vs_environment/tf_flying_mob.h"

class CTFMobGenerator : public CPointEntity
{
public:
	DECLARE_CLASS( CTFMobGenerator, CPointEntity );
	DECLARE_DATADESC();

	CTFMobGenerator( void );
	virtual ~CTFMobGenerator() { }

	virtual void Activate();

	void GeneratorThink( void );
	void SpawnMob( void );

	// Input.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSpawnMob( inputdata_t &inputdata );
	void InputRemoveMobs( inputdata_t &inputdata );

	// Output
	void OnMobKilled( NextBotCombatCharacter *pMob );

private:
	int m_spawnCount;
	int m_maxActiveCount;
	float m_spawnInterval;
	MobType_t m_mobType;
	bool m_bSpawnOnlyWhenTriggered;
	int m_spawnCountRemaining;
	bool m_bExpended;
	bool m_bEnabled;

	COutputEvent m_onMobSpawned;
	COutputEvent m_onMobKilled;
	COutputEvent m_onExpended;

	CUtlVector< CHandle< NextBotCombatCharacter > > m_spawnedMobVector;
};

#endif // TF_MOB_GENERATOR_H
