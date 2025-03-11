//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MAINMENUOVERRIDE_H
#define TF_HUD_MAINMENUOVERRIDE_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ScrollableEditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_shareddefs.h"
#include "vgui_avatarimage.h"
#include "tf_imagepanel.h"
#include "tf_gamestats_shared.h"
#include "tf_controls.h"
#include "item_model_panel.h"
#include "gcsdk/gcclientsdk.h"
#include "local_steam_shared_object_listener.h"


#include "mute_player_dialog.h"

using namespace vgui;
using namespace GCSDK;

class CExButton;
class HTML;

enum mm_button_styles
{
	MMBS_NORMAL = 0,
	MMBS_SUBBUTTON = 1,
	MMBS_CUSTOM,
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudMainMenuOverride : public vgui::EditablePanel, public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CHudMainMenuOverride, vgui::EditablePanel );

	enum mm_highlight_anims
	{
		MMHA_TUTORIAL = 0,
		MMHA_PRACTICE,
		MMHA_NEWUSERFORUM,
		MMHA_OPTIONS,
		MMHA_LOADOUT,
		MMHA_STORE,

		NUM_ANIMS
	};

public:
	CHudMainMenuOverride( IViewPort *pViewPort );
	~CHudMainMenuOverride( void );

	void		 AttachToGameUI( void );
	virtual const char *GetName( void ){ return PANEL_MAINMENUOVERRIDE; }
	virtual void SetData( KeyValues *data ){}
	virtual void Reset(){ Update(); SetVisible( true ); }
	virtual void Update() { return; }
	virtual bool NeedsUpdate( void ){ return false; }
	virtual bool HasInputElements( void ){ return true; }
	virtual void ShowPanel( bool bShow ) { SetVisible( true ); }	// Refuses to hide

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible();
	virtual void SetParent( vgui::VPANEL parent ){ BaseClass::SetParent( parent ); }

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( IScheme *scheme );
	virtual void PerformLayout( void );

	void OnCommand( const char *command );

	void OnKeyCodePressed( KeyCode code );

	void		 LoadMenuEntries( void );
	void		 RemoveAllMenuEntries( void );
	virtual void FireGameEvent( IGameEvent *event );

	void		 LoadCharacterImageFile( void );

	void		 UpdateNotifications();
	void		 SetNotificationsButtonVisible( bool bVisible );
	void		 SetNotificationsPanelVisible( bool bVisible );
	void		 AdjustNotificationsPanelHeight();

	void		 UpdatePromotionalCodes( void );

	CExplanationPopup*	 StartHighlightAnimation( mm_highlight_anims iAnim );

	MESSAGE_FUNC( OnUpdateMenu, "UpdateMenu" );
	MESSAGE_FUNC( OnMainMenuStabilized, "MainMenuStabilized" );

	void		ScheduleItemCheck( void ) { m_flCheckUnclaimedItems = (engine->Time() + 1.5); }

	void		CheckUnclaimedItems();

	void		OnTick();

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; } // Seems like this should be GAME_ACTION_SET_MENU, but it's not because it's apparently visible *all* *the* *time*

#ifdef _DEBUG
	void		Refresh();
#endif


protected:
	virtual void PaintTraverse( bool Repaint, bool allowForce = true ) OVERRIDE;

private:

	void SOEvent( const CSharedObject* pObject );

	void		PerformKeyRebindings( void );

	bool		CheckAndWarnForPREC( void );
	void		StopUpdateGlow();

private:

	// Notifications
	vgui::EditablePanel				*m_pNotificationsShowPanel;
	vgui::EditablePanel				*m_pNotificationsPanel;
	vgui::EditablePanel				*m_pNotificationsControl;
	vgui::ScrollableEditablePanel	*m_pNotificationsScroller;
	int								m_iNumNotifications;
	int								m_iNotiPanelWide;

	vgui::ImagePanel		*m_pCharacterImagePanel;
	int						 m_iCharacterImageIdx;

	CExButton				*m_pQuitButton;
	CExButton				*m_pDisconnectButton;
	bool					m_bIsDisconnectText;

	CExButton				*m_pBackToReplaysButton;
	ImagePanel				*m_pStoreHasNewItemsImage;
	CExButton				*m_pStoreButton;

	KeyValues				*m_pButtonKV;
	bool					m_bReapplyButtonKVs;

	float					m_flCheckUnclaimedItems;

	vgui::ImagePanel		*m_pBackground;

	struct mainmenu_entry_t
	{
		vgui::EditablePanel *pPanel;
		bool		bOnlyInGame;
		bool		bOnlyInReplay;
		bool		bOnlyAtMenu;
		bool		bIsVisible;
		bool		bOnlyVREnabled;
		int			iStyle;
		const char	*pszImage;
		const char	*pszTooltip;
	};
	CUtlVector<mainmenu_entry_t>	m_pMMButtonEntries;

	CMainMenuToolTip		*m_pToolTip;
	vgui::EditablePanel		*m_pToolTipEmbeddedPanel;

	EditablePanel	*m_pEventPromoContainer;
	EditablePanel	*m_pSafeModeContainer;

	vgui::DHANDLE<CMutePlayerDialog> m_hMutePlayerDialog;

	bool m_bStabilizedInitialLayout;
	bool m_bBackgroundUsesCharacterImages;
	const char* m_pszForcedCharacterImage = NULL;

	CPanelAnimationVarAliasType( int, m_iButtonXOffset, "button_x_offset", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonY, "button_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iButtonYDelta, "button_y_delta", "0", "proportional_int" );
};

#endif //TF_HUD_MAINMENUOVERRIDE_H
