//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#ifndef TF_MOB_SPAWN_H
#define TF_MOB_SPAWN_H

class CMobSpawner : public CPointEntity
{
	DECLARE_CLASS( CMobSpawner, CPointEntity );
	DECLARE_DATADESC();
public:
	CMobSpawner();

	virtual void Spawn();
	virtual void Think();

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputSetMaxActiveMobs( inputdata_t &inputdata );

private:
	bool m_bEnabled;
	bool m_bInfiniteMeleeMobs;
	int m_nMaxActiveMobs;
	float m_flMeleeMobLifeTime;
	int m_nMobType;

	int m_nSpawned;

	CUtlVector< CHandle< CTFMeleeMob > > m_activeMeleeMobs;
};

#endif // TF_MOB_SPAWN_H
