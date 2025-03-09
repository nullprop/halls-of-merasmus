// tf_mob_generator.h
// Entity to spawn a collection of Mobs

#ifndef TF_MOB_GENERATOR_H
#define TF_MOB_GENERATOR_H

#include "player_vs_environment/tf_melee_mob.h"

class CTFMobGenerator : public CPointEntity
{
public:
	DECLARE_CLASS( CTFMobGenerator, CPointEntity );
	DECLARE_DATADESC();

	CTFMobGenerator( void );
	virtual ~CTFMobGenerator() { }

	virtual void Activate();

	void GeneratorThink( void );
	void SpawnBot( void );

	// Input.
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSpawnBot( inputdata_t &inputdata );
	void InputRemoveBots( inputdata_t &inputdata );

	// Output
	void OnBotKilled( CTFMeleeMob *pBot );

private:
	int m_spawnCount;
	int m_maxActiveCount;
	float m_spawnInterval;
	bool m_bSpawnOnlyWhenTriggered;
	MobType_t m_mobType;
	int m_spawnCountRemaining;
	bool m_bExpended;
	bool m_bEnabled;

	COutputEvent m_onSpawned;
	COutputEvent m_onExpended;
	COutputEvent m_onBotKilled;

	CUtlVector< CHandle< CTFMeleeMob > > m_spawnedBotVector;
};

#endif // TF_MOB_GENERATOR_H
