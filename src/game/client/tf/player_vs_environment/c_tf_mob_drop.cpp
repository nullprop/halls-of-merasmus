//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"

#include "c_tf_mob_drop.h"
#include "c_tf_player.h"

IMPLEMENT_CLIENTCLASS_DT( C_TFMobDrop, DT_MobDrop, CTFMobDrop )
//	RecvPropBool( RECVINFO( m_bDistributed ) ),
END_RECV_TABLE()


C_TFMobDrop::C_TFMobDrop()
{
	m_pGlowEffect = NULL;
	m_bShouldGlowForLocalPlayer = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TFMobDrop::~C_TFMobDrop()
{
	DestroyGlowEffect();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFMobDrop::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFMobDrop::ClientThink()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFMobDrop::UpdateGlowEffect( void )
{
	// destroy the existing effect
	if ( m_pGlowEffect )
	{
		DestroyGlowEffect();
	}

	// create a new effect if we have a cart
	if ( m_bShouldGlowForLocalPlayer )
	{
		Vector color = Vector( 0, 150, 0 );
		m_pGlowEffect = new CGlowObject( this, color, 1.0, true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFMobDrop::DestroyGlowEffect( void )
{
	if ( m_pGlowEffect )
	{
		delete m_pGlowEffect;
		m_pGlowEffect = NULL;
	}
}
