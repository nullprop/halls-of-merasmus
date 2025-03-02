//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef C_TF_MOB_DROP_H
#define C_TF_MOB_DROP_H

class C_TFMobDrop : public C_BaseAnimating
{
public:
	DECLARE_CLASS( C_TFMobDrop, C_BaseAnimating );
	DECLARE_CLIENTCLASS();

	C_TFMobDrop();
	~C_TFMobDrop();

	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void ClientThink();

private:

	void UpdateGlowEffect( void );
	void DestroyGlowEffect( void );
	CGlowObject *m_pGlowEffect;
	bool m_bShouldGlowForLocalPlayer;
};

#endif // C_TF_MOB_DROP_H
