//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "tf_hud_mainmenuoverride.h"
#include "ienginevgui.h"
#include "tf_viewport.h"
#include "clientmode_tf.h"
#include "confirm_dialog.h"
#include <vgui/ILocalize.h>
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "tf_statsummary.h"
#include "rtime.h"
#include "tf_gcmessages.h"
#include "econ_notifications.h"
#include "c_tf_freeaccount.h"
#include "econ_gcmessages.h"
#include "econ_item_inventory.h"
#include "gcsdk/gcclient.h"
#include "gcsdk/gcclientjob.h"
#include "econ_item_system.h"
#include <vgui_controls/AnimationController.h>
#include "store/store_panel.h"
#include "gc_clientsystem.h"
#include <vgui_controls/ScrollBarSlider.h>
#include "filesystem.h"
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_partyclient.h"
#include "sourcevr/isourcevirtualreality.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/materialsystem_config.h"
#include "tf_warinfopanel.h"
#include "tf_item_inventory.h"
#include "tf_matchmaking_shared.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "tf_rating_data.h"
#include "tf_progression.h"

#include "replay/ireplaysystem.h"
#include "replay/ienginereplay.h"
#include "replay/vgui/replayperformanceeditor.h"
#include "materialsystem/itexture.h"
#include "imageutils.h"
#include "icommandline.h"
#include "vgui/ISystem.h"
#include "mute_player_dialog.h"
#include "tf_matchmaking_dashboard.h"

#include "econ_paintkit.h"
#include "ienginevgui.h"


#include "c_tf_gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );

extern const char *g_sImagesBlue[];
extern int EconWear_ToIntCategory( float flWear );

void cc_tf_safemode_toggle( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	CHudMainMenuOverride *pMMOverride = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
	if ( pMMOverride )
	{
		pMMOverride->InvalidateLayout();
	}
}

ConVar tf_recent_achievements( "tf_recent_achievements", "0", FCVAR_ARCHIVE );
ConVar tf_find_a_match_hint_viewed( "tf_find_a_match_hint_viewed", "0", FCVAR_ARCHIVE );
ConVar cl_ask_bigpicture_controller_opt_out( "cl_ask_bigpicture_controller_opt_out", "0", FCVAR_ARCHIVE, "Whether the user has opted out of being prompted for controller support in Big Picture." );
ConVar cl_mainmenu_safemode( "cl_mainmenu_safemode", "0", FCVAR_NONE, "Enable safe mode", cc_tf_safemode_toggle );
ConVar cl_mainmenu_updateglow( "cl_mainmenu_updateglow", "1", FCVAR_ARCHIVE | FCVAR_HIDDEN );

void cc_promotional_codes_button_changed( IConVar *pConVar, const char *pOldString, float flOldValue )
{
	IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
	if ( pMMOverride )
	{
		( (CHudMainMenuOverride*)pMMOverride )->UpdatePromotionalCodes();
	}
}
ConVar cl_promotional_codes_button_show( "cl_promotional_codes_button_show", "1", FCVAR_ARCHIVE, "Toggles the 'View Promotional Codes' button in the main menu for players that have used the 'RIFT Well Spun Hat Claim Code'.", cc_promotional_codes_button_changed );

void PromptOrFireCommand( const char* pszCommand )
{
	if ( engine->IsInGame()  )
	{
		CTFDisconnectConfirmDialog *pDialog = BuildDisconnectConfirmDialog();
		if ( pDialog )
		{
			pDialog->Show();
			pDialog->AddConfirmCommand( pszCommand );
		}
	}
	else
	{
		engine->ClientCmd_Unrestricted( pszCommand );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CHudMainMenuOverride::CHudMainMenuOverride( IViewPort *pViewPort ) : BaseClass( NULL, PANEL_MAINMENUOVERRIDE )
{
	// We don't want the gameui to delete us, or things get messy
	SetAutoDelete( false );
	SetVisible( true );

	m_pButtonKV = NULL;
	m_pQuitButton = NULL;
	m_pDisconnectButton = NULL;
	m_pBackToReplaysButton = NULL;
	m_pStoreHasNewItemsImage = NULL;

	m_iNotiPanelWide = 0;

	m_bReapplyButtonKVs = false;

	m_iCharacterImageIdx = -1;

	ScheduleItemCheck();

 	m_pToolTip = new CMainMenuToolTip( this );
 	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTipEmbeddedPanel->MoveToFront();
 	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );

	ListenForGameEvent( "gc_new_session" );
	ListenForGameEvent( "item_schema_initialized" );
	ListenForGameEvent( "store_pricesheet_updated" );
	ListenForGameEvent( "gameui_activated" );
	ListenForGameEvent( "party_updated" );
	ListenForGameEvent( "server_spawn" );

	m_pNotificationsShowPanel = NULL;
	m_pNotificationsPanel = new vgui::EditablePanel( this, "Notifications_Panel" );
	m_pNotificationsControl = NotificationQueue_CreateMainMenuUIElement( m_pNotificationsPanel, "Notifications_Control" );
	m_pNotificationsScroller = new vgui::ScrollableEditablePanel( m_pNotificationsPanel, m_pNotificationsControl, "Notifications_Scroller" );

	m_pNotificationsPanel->SetVisible(false);
	m_pNotificationsControl->SetVisible(false);
	m_pNotificationsScroller->SetVisible(false);

	m_iNumNotifications = 0;

	m_pBackground = new vgui::ImagePanel( this, "Background" );
	m_pEventPromoContainer = new EditablePanel( this, "EventPromo" );
	m_pSafeModeContainer = new EditablePanel( this, "SafeMode" );

	m_bStabilizedInitialLayout = false;

	m_bBackgroundUsesCharacterImages = true;

	m_pCharacterImagePanel = new ImagePanel( this, "TFCharacterImage" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CHudMainMenuOverride::~CHudMainMenuOverride( void )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_close", "main_menu_override" );

	if ( GetClientModeTFNormal()->GameUI() )
	{
		GetClientModeTFNormal()->GameUI()->SetMainMenuOverride( NULL );
	}

	if ( m_pButtonKV )
	{
		m_pButtonKV->deleteThis();
		m_pButtonKV = NULL;
	}

	// Stop Animation Sequences
	if ( m_pNotificationsShowPanel )
	{
		g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel( m_pNotificationsShowPanel );
	}

	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: Override painting traversal to suppress main menu painting if we're not ready to show yet
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::PaintTraverse( bool Repaint, bool allowForce )
{
	// Ugly hack: disable painting until we're done screwing around with updating the layout during initialization.
	// Use -menupaintduringinit command line parameter to reinstate old behavior
	if ( m_bStabilizedInitialLayout || CommandLine()->CheckParm("-menupaintduringinit") )
	{
		BaseClass::PaintTraverse( Repaint, allowForce );
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnTick()
{
	if ( m_iNumNotifications != NotificationQueue_GetNumMainMenuNotifications() )
	{
		m_iNumNotifications = NotificationQueue_GetNumMainMenuNotifications();
		UpdateNotifications();
	}
	else if ( m_pNotificationsPanel->IsVisible() )
	{
		AdjustNotificationsPanelHeight();
	}

	static bool s_bRanOnce = false;
	if ( !s_bRanOnce )
	{
		s_bRanOnce = true;
		if ( char const *szConnectAdr = CommandLine()->ParmValue( "+connect" ) )
		{
			Msg( "Executing deferred connect command: %s\n", szConnectAdr );
			engine->ExecuteClientCmd( CFmtStr( "connect %s -%s\n", szConnectAdr, "ConnectStringOnCommandline" ) );
		}
	}


}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::AttachToGameUI( void )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "interface_open",	"main_menu_override" );

	if ( GetClientModeTFNormal()->GameUI() )
	{
		GetClientModeTFNormal()->GameUI()->SetMainMenuOverride( GetVPanel() );
	}

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );
	SetCursor(dc_arrow);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
ConVar tf_last_store_pricesheet_version( "tf_last_store_pricesheet_version", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_DONTRECORD | FCVAR_HIDDEN );

void CHudMainMenuOverride::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp( type, "store_pricesheet_updated" ) == 0 )
	{
		// If the contents of the store have changed since the last time we went in and/or launched
		// the game, change the button color so that players know there's new content available.
		if ( EconUI() &&
			 EconUI()->GetStorePanel() &&
			 EconUI()->GetStorePanel()->GetPriceSheet() )
		{
			const CEconStorePriceSheet *pPriceSheet = EconUI()->GetStorePanel()->GetPriceSheet();

			// The cvar system can't deal with integers that lose data when represented as floating point
			// numbers. We don't really care about supreme accuracy for detecting changes -- worst case if
			// we change the price sheet almost exactly 18 hours apart, some subset of players won't get the
			// "new!" label and that's fine.
			const uint32 unPriceSheetVersion = (uint32)pPriceSheet->GetVersionStamp() & 0xffff;

			if ( unPriceSheetVersion != (uint32)tf_last_store_pricesheet_version.GetInt() )
			{
				tf_last_store_pricesheet_version.SetValue( (int)unPriceSheetVersion );

				if ( m_pStoreHasNewItemsImage )
				{
					m_pStoreHasNewItemsImage->SetVisible( true );
				}
			}
		}

		// might as well do this here too
		UpdatePromotionalCodes();

		LoadCharacterImageFile();

		if ( NeedsToChooseMostHelpfulFriend() )
		{
			NotifyNeedsToChooseMostHelpfulFriend();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "button_kv" );
	if ( pItemKV )
	{
		if ( m_pButtonKV )
		{
			m_pButtonKV->deleteThis();
		}
		m_pButtonKV = new KeyValues("button_kv");
		pItemKV->CopySubkeys( m_pButtonKV );

		m_bReapplyButtonKVs = true;
	}

}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::ApplySchemeSettings( IScheme *scheme )
{
	// We need to re-hook ourselves up to the TF client scheme, because the GameUI will try to change us their its scheme
	vgui::HScheme pScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(pScheme);
	SetProportional( true );

	m_bBackgroundUsesCharacterImages = true;
	m_pszForcedCharacterImage = NULL;

	KeyValues *pConditions = new KeyValues( "conditions" );

	// spooky!
	AddSubKeyNamed( pConditions, "if_halloween" );

	// Pick a random background
	int nBackground = RandomInt( 0, 5 );
	AddSubKeyNamed( pConditions, CFmtStr( "if_halloween_%d", nBackground ) );
	if ( ( nBackground == 3 ) || ( nBackground == 4 ) )
	{
		m_bBackgroundUsesCharacterImages = false;
	}

	// Put in ratio condition
	float aspectRatio = engine->GetScreenAspectRatio();
	AddSubKeyNamed( pConditions, aspectRatio >= 1.6 ? "if_wider" : "if_taller" );

	RemoveAllMenuEntries();

	LoadControlSettings( "resource/UI/MainMenuOverride.res", NULL, NULL, pConditions );

	BaseClass::ApplySchemeSettings( vgui::scheme()->GetIScheme(pScheme) );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_pQuitButton = dynamic_cast<CExButton*>( FindChildByName("QuitButton") );
	m_pDisconnectButton = dynamic_cast<CExButton*>( FindChildByName("DisconnectButton") );
	m_pBackToReplaysButton = dynamic_cast<CExButton*>( FindChildByName("BackToReplaysButton") );
	m_pStoreHasNewItemsImage = dynamic_cast<ImagePanel*>( FindChildByName( "StoreHasNewItemsImage", true ) );
	m_pStoreButton = dynamic_cast<CExButton*>(FindChildByName("GeneralStoreButton"));
	if (m_pStoreButton)
	{
		m_pStoreButton->SetVisible(false);
	}

	m_bIsDisconnectText = true;

	// m_pNotificationsShowPanel shows number of unread notifications. Pressing it pops up the first notification.
	m_pNotificationsShowPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("Notifications_ShowButtonPanel") );

	m_pNotificationsShowPanel->SetVisible(false);

	m_pNotificationsScroller->GetScrollbar()->SetAutohideButtons( true );
	m_pNotificationsScroller->GetScrollbar()->SetPaintBorderEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->SetPaintBackgroundEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(0)->SetPaintBorderEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(0)->SetPaintBackgroundEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(1)->SetPaintBorderEnabled( false );
	m_pNotificationsScroller->GetScrollbar()->GetButton(1)->SetPaintBackgroundEnabled( false );

	// Add tooltips for various buttons
	auto lambdaAddTooltip = [&]( const char* pszPanelName, const char* pszTooltipText )
	{
		Panel* pPanelToAddTooltipTipTo = FindChildByName( pszPanelName );
		if ( pPanelToAddTooltipTipTo)
		{
			pPanelToAddTooltipTipTo->SetTooltip( m_pToolTip, pszTooltipText );

			pPanelToAddTooltipTipTo->SetVisible(false);
		}
	};

	lambdaAddTooltip( "CommentaryButton", "#MMenu_Tooltip_Commentary" );
	lambdaAddTooltip( "CoachPlayersButton", "#MMenu_Tooltip_Coach" );
	lambdaAddTooltip( "ReportBugButton", "#MMenu_Tooltip_ReportBug" );
	lambdaAddTooltip( "AchievementsButton", "#MMenu_Tooltip_Achievements" );
	lambdaAddTooltip( "NewUserForumsButton", "#MMenu_Tooltip_NewUserForum" );
	lambdaAddTooltip( "ReplayButton", "#MMenu_Tooltip_Replay" );
	lambdaAddTooltip( "WorkshopButton", "#MMenu_Tooltip_Workshop" );
	lambdaAddTooltip( "SettingsButton", "#MMenu_Tooltip_Options" );
	lambdaAddTooltip( "TF2SettingsButton", "#MMenu_Tooltip_AdvOptions" );


	LoadCharacterImageFile();

	RemoveAllMenuEntries();
	LoadMenuEntries();

	UpdateNotifications();
	UpdatePromotionalCodes();

	PerformKeyRebindings();

	GetMMDashboard();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::LoadCharacterImageFile( void )
{
	m_pCharacterImagePanel->SetVisible( m_bBackgroundUsesCharacterImages );

	if ( !m_bBackgroundUsesCharacterImages )
	{
		return;
	}

	// If we've got a forced image, use that
	if ( m_pszForcedCharacterImage && *m_pszForcedCharacterImage )
	{
		m_pCharacterImagePanel->SetImage( m_pszForcedCharacterImage );
		return;
	}

	KeyValues *pCharacterFile = new KeyValues( "CharacterBackgrounds" );

	if ( pCharacterFile->LoadFromFile( g_pFullFileSystem, "scripts/CharacterBackgrounds.txt" ) )
	{
		CUtlVector<KeyValues *> vecUseableCharacters;

		const char* pszActiveWarName = NULL;
		const WarDefinitionMap_t& mapWars = GetItemSchema()->GetWarDefinitions();
		FOR_EACH_MAP_FAST( mapWars, i )
		{
			const CWarDefinition* pWarDef = mapWars[i];
			if ( pWarDef->IsActive() )
			{
				pszActiveWarName = pWarDef->GetDefName();
				break;
			}
		}

		bool bActiveOperation = false;

		// Uncomment if another operation happens
		//FOR_EACH_MAP_FAST( GetItemSchema()->GetOperationDefinitions(), iOperation )
		//{
		//	CEconOperationDefinition *pOperation = GetItemSchema()->GetOperationDefinitions()[iOperation];
		//	if ( !pOperation || !pOperation->IsActive() || !pOperation->IsCampaign() )
		//		continue;

		//	bActiveOperation = true;
		//	break;
		//}

		// Count the number of possible characters.
		FOR_EACH_SUBKEY( pCharacterFile, pCharacter )
		{
			bool bIsOperationCharacter = bActiveOperation && pCharacter->GetBool( "operation", false );

			EHoliday eHoliday = (EHoliday)UTIL_GetHolidayForString( pCharacter->GetString( "holiday_restriction" ) );


			const char* pszAssociatedWar = pCharacter->GetString( "war_restriction" );

			int iWeight = pCharacter->GetInt( "weight", 1 );

			// If a War is active, that's all we want to show.  If not, then bias towards holidays
			if ( pszActiveWarName != NULL )
			{
				if ( !FStrEq( pszAssociatedWar, pszActiveWarName ) )
				{
					iWeight = 0;
				}
			}
			else if ( eHoliday != kHoliday_None )
			{
				iWeight = UTIL_IsHolidayActive( eHoliday ) ? MAX( iWeight, 6 ) : 0;
			}
			else if ( bActiveOperation && !bIsOperationCharacter )
			{
				iWeight = 0;
			}
			else
			{
				// special cases for summer, halloween, fullmoon, and christmas...turn off anything not covered above
				if ( UTIL_IsHolidayActive( kHoliday_Summer ) || UTIL_IsHolidayActive( kHoliday_HalloweenOrFullMoon ) || UTIL_IsHolidayActive( kHoliday_Christmas ) )
				{
					iWeight = 0;
				}
			}

			for ( int i = 0; i < iWeight; i++ )
			{
				vecUseableCharacters.AddToTail( pCharacter );
			}
		}

		// Pick a character at random.
		if ( vecUseableCharacters.Count() > 0 )
		{
			m_iCharacterImageIdx = rand() % vecUseableCharacters.Count();
		}

		// Make sure we found a character we can use.
		if ( vecUseableCharacters.IsValidIndex( m_iCharacterImageIdx ) )
		{
			KeyValues *pCharacter = vecUseableCharacters[m_iCharacterImageIdx];
			const char* image_name = pCharacter->GetString( "image" );
			m_pCharacterImagePanel->SetImage( image_name );
		}
	}

	pCharacterFile->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::LoadMenuEntries( void )
{
	KeyValues *datafile = new KeyValues("GameMenu");
	datafile->UsesEscapeSequences( true );	// VGUI uses escape sequences
	bool bLoaded = datafile->LoadFromFile( g_pFullFileSystem, "Resource/GameMenu.res", "custom_mod" );
	if ( !bLoaded )
	{
		bLoaded = datafile->LoadFromFile( g_pFullFileSystem, "Resource/GameMenu.res", "vgui" );
		if ( !bLoaded )
		{
			// only allow to load loose files when using insecure mode
			if ( CommandLine()->FindParm( "-insecure" ) )
			{
				bLoaded = datafile->LoadFromFile( g_pFullFileSystem, "Resource/GameMenu.res" );
			}
		}
	}

	for (KeyValues *dat = datafile->GetFirstSubKey(); dat != NULL; dat = dat->GetNextKey())
	{
		const char *label = dat->GetString("label", "<unknown>");
		const char *cmd = dat->GetString("command", NULL);
		const char *name = dat->GetName();
		int iStyle = dat->GetInt("style", 0 );

		if ( !cmd || !cmd[0] )
		{
			int iIdx = m_pMMButtonEntries.AddToTail();
			m_pMMButtonEntries[iIdx].pPanel = NULL;
			m_pMMButtonEntries[iIdx].bOnlyInGame = dat->GetBool( "OnlyInGame" );
			m_pMMButtonEntries[iIdx].bOnlyInReplay = dat->GetBool( "OnlyInReplay" );
			m_pMMButtonEntries[iIdx].bOnlyAtMenu = dat->GetBool( "OnlyAtMenu" );
			m_pMMButtonEntries[iIdx].bOnlyVREnabled = dat->GetBool( "OnlyWhenVREnabled" );
			m_pMMButtonEntries[iIdx].iStyle = iStyle;
			continue;
		}

		// Create the new editable panel (first, see if we have one already)
		vgui::EditablePanel *pPanel = dynamic_cast<vgui::EditablePanel *>( FindChildByName( name, true ) );
		if ( !pPanel )
		{
			// We don't want to do this anymore.  We need an actual hierarchy so things can slide
			// around when the play buttin is pressed and the play options expand.
			AssertMsg1( false, "Could not find panel %s\n", name );
			pPanel = new vgui::EditablePanel( this, name );
		}
		else
		{
			// It already exists in our .res file. Note that it's a custom button.
			iStyle = MMBS_CUSTOM;
		}

		if ( pPanel )
		{
			if ( m_pButtonKV && iStyle != MMBS_CUSTOM )
			{
				pPanel->ApplySettings( m_pButtonKV );
			}

			int iIdx = m_pMMButtonEntries.AddToTail();
			m_pMMButtonEntries[iIdx].pPanel = pPanel;
			m_pMMButtonEntries[iIdx].bOnlyInGame = dat->GetBool( "OnlyInGame" );
			m_pMMButtonEntries[iIdx].bOnlyInReplay = dat->GetBool( "OnlyInReplay" );
			m_pMMButtonEntries[iIdx].bOnlyAtMenu = dat->GetBool( "OnlyAtMenu" );
			m_pMMButtonEntries[iIdx].bOnlyVREnabled = dat->GetBool( "OnlyWhenVREnabled" );
			m_pMMButtonEntries[iIdx].iStyle = iStyle;
			m_pMMButtonEntries[iIdx].pszImage = dat->GetString( "subimage" );
			m_pMMButtonEntries[iIdx].pszTooltip = dat->GetString( "tooltip", NULL );

			// Tell the button that we'd like messages from it
			CExImageButton *pButton = dynamic_cast<CExImageButton*>( pPanel->FindChildByName("SubButton") );
			if ( pButton )
			{
				if ( m_pMMButtonEntries[iIdx].pszTooltip )
				{
					pButton->SetTooltip( m_pToolTip, m_pMMButtonEntries[iIdx].pszTooltip );
				}

				pButton->SetText( label );
				pButton->SetCommand( cmd );
				pButton->SetMouseInputEnabled( true );
				pButton->AddActionSignalTarget( GetVPanel() );

				if ( m_pMMButtonEntries[iIdx].pszImage && m_pMMButtonEntries[iIdx].pszImage[0] )
				{
					pButton->SetSubImage( m_pMMButtonEntries[iIdx].pszImage );
				}
			}
		}

		OnUpdateMenu();
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::RemoveAllMenuEntries( void )
{
	FOR_EACH_VEC_BACK( m_pMMButtonEntries, i )
	{
		if ( m_pMMButtonEntries[i].pPanel )
		{
			// Manually remove anything that's not going to be removed automatically
			if ( m_pMMButtonEntries[i].pPanel->IsBuildModeDeletable() == false )
			{
				m_pMMButtonEntries[i].pPanel->MarkForDeletion();
			}
		}
	}
	m_pMMButtonEntries.Purge();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::PerformLayout( void )
{
	BaseClass::PerformLayout();

	bool bFirstButton = true;

	int iYPos = m_iButtonY;
	FOR_EACH_VEC( m_pMMButtonEntries, i )
	{
		bool bIsVisible = (m_pMMButtonEntries[i].pPanel ? m_pMMButtonEntries[i].pPanel->IsVisible() : m_pMMButtonEntries[i].bIsVisible);
		if ( !bIsVisible )
			continue;

		if ( bFirstButton && m_pMMButtonEntries[i].pPanel != NULL )
		{
			m_pMMButtonEntries[i].pPanel->NavigateTo();
			bFirstButton = false;
		}

		// Don't reposition it if it's a custom button
		if ( m_pMMButtonEntries[i].iStyle == MMBS_CUSTOM )
			continue;

		// If we're a spacer, just leave a blank and move on
		if ( m_pMMButtonEntries[i].pPanel == NULL )
		{
			iYPos += YRES(20);
			continue;
		}

		m_pMMButtonEntries[i].pPanel->SetPos( (GetWide() * 0.5) + m_iButtonXOffset, iYPos );
		iYPos += m_pMMButtonEntries[i].pPanel->GetTall() + m_iButtonYDelta;
	}

	if ( m_pEventPromoContainer && m_pSafeModeContainer )
	{
		m_pEventPromoContainer->SetVisible( !cl_mainmenu_safemode.GetBool() );
		m_pSafeModeContainer->SetVisible( cl_mainmenu_safemode.GetBool() );
		if ( cl_mainmenu_safemode.GetBool() )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pSafeModeContainer, "MMenu_SafeMode_Blink" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->CancelAnimationsForPanel( m_pSafeModeContainer );
		}
	}

	// Make the glows behind the update buttons pulse
	if ( m_pEventPromoContainer && cl_mainmenu_updateglow.GetInt() )
	{
		EditablePanel* pUpdateBackground = m_pEventPromoContainer->FindControl< EditablePanel >( "Background", true );
		if ( pUpdateBackground )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pUpdateBackground, "MMenu_UpdateButton_StartGlow" );
		}
	}

	m_pEventPromoContainer->SetVisible(false);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnUpdateMenu( void )
{
	// The dumb gameui.dll basepanel calls this every frame it's visible.
	// So try and do the least amount of work if nothing has changed.

	bool bSomethingChanged = false;
	bool bInGame = engine->IsInGame();
#if defined( REPLAY_ENABLED )
	bool bInReplay = g_pEngineClientReplay->IsPlayingReplayDemo();
#else
	bool bInReplay = false;
#endif

	// First, reapply any KVs we have to reapply
	if ( m_bReapplyButtonKVs )
	{
		m_bReapplyButtonKVs = false;

		if ( m_pButtonKV )
		{
			FOR_EACH_VEC( m_pMMButtonEntries, i )
			{
				if ( m_pMMButtonEntries[i].iStyle != MMBS_CUSTOM && m_pMMButtonEntries[i].pPanel )
				{
					m_pMMButtonEntries[i].pPanel->ApplySettings( m_pButtonKV );
				}
			}
		}
	}

	// Hide the character if we're in game.
	if ( bInGame || bInReplay )
	{
		if ( m_pCharacterImagePanel->IsVisible() )
		{
			m_pCharacterImagePanel->SetVisible( false );
		}
	}
	else if ( !bInGame && !bInReplay )
	{
		if ( !m_pCharacterImagePanel->IsVisible() )
		{
			m_pCharacterImagePanel->SetVisible( m_bBackgroundUsesCharacterImages );
		}
	}

	// Position the entries
	FOR_EACH_VEC( m_pMMButtonEntries, i )
	{
		bool shouldBeVisible = true;
		if ( m_pMMButtonEntries[i].bOnlyInGame && !bInGame )
		{
			shouldBeVisible = false;
		}
		else if ( m_pMMButtonEntries[i].bOnlyInReplay && !bInReplay )
		{
			shouldBeVisible = false;
		}
		else if ( m_pMMButtonEntries[i].bOnlyAtMenu && (bInGame || bInReplay) )
		{
			shouldBeVisible = false;
		}
		else if ( m_pMMButtonEntries[i].bOnlyVREnabled )
		{
			shouldBeVisible = false;
		}

		// Set the right visibility
		bool bIsVisible = (m_pMMButtonEntries[i].pPanel ? m_pMMButtonEntries[i].pPanel->IsVisible() : m_pMMButtonEntries[i].bIsVisible);
		if ( bIsVisible != shouldBeVisible )
		{
			m_pMMButtonEntries[i].bIsVisible = shouldBeVisible;
			if ( m_pMMButtonEntries[i].pPanel )
			{
				m_pMMButtonEntries[i].pPanel->SetVisible( shouldBeVisible );
			}
			bSomethingChanged = true;
		}
	}

	if ( m_pQuitButton && m_pDisconnectButton && m_pBackToReplaysButton )
	{
		bool bShowQuit = !( bInGame || bInReplay );
		bool bShowDisconnect = bInGame && !bInReplay;

		if ( m_pQuitButton->IsVisible() != bShowQuit )
		{
			m_pQuitButton->SetVisible( bShowQuit );
		}

		if ( m_pBackToReplaysButton->IsVisible() != bInReplay )
		{
			m_pBackToReplaysButton->SetVisible( bInReplay );
		}

		if ( m_pDisconnectButton->IsVisible() != bShowDisconnect )
		{
			m_pDisconnectButton->SetVisible( bShowDisconnect );
		}

		if ( bShowDisconnect )
		{
			bool bIsDisconnectText = GTFGCClientSystem()->GetCurrentServerAbandonStatus() != k_EAbandonGameStatus_AbandonWithPenalty;
			if ( m_bIsDisconnectText != bIsDisconnectText )
			{
				m_bIsDisconnectText = bIsDisconnectText;
				m_pDisconnectButton->SetText( m_bIsDisconnectText ? "#GameUI_GameMenu_Disconnect" : "#TF_MM_Rejoin_Abandon" );
			}
		}
	}

	if ( m_pBackground )
	{
		bool bShouldBeVisible = bInGame == false;
		if ( m_pBackground->IsVisible() != bShouldBeVisible )
		{
			m_pBackground->SetVisible( bShouldBeVisible );
		}
	}

	if ( bSomethingChanged )
	{
		InvalidateLayout();

		ScheduleItemCheck();
	}

	if ( !bInGame && m_flCheckUnclaimedItems && m_flCheckUnclaimedItems < engine->Time() )
	{
		m_flCheckUnclaimedItems = 0;
		CheckUnclaimedItems();
	}

	if ( !IsLayoutInvalid() )
	{
		if ( !m_bStabilizedInitialLayout )
		{
			PostMessage( this, new KeyValues( "MainMenuStabilized" ), 2.f );
		}

		m_bStabilizedInitialLayout = true;
	}
}

void CHudMainMenuOverride::OnMainMenuStabilized()
{
	IGameEvent *event = gameeventmanager->CreateEvent( "mainmenu_stabilized" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if we need to hound the player about unclaimed items.
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::CheckUnclaimedItems()
{
	// Only do this if we don't have a notification about unclaimed items already.
	for ( int i=0; i<NotificationQueue_GetNumNotifications(); i++ )
	{
		CEconNotification* pNotification = NotificationQueue_Get( i );
		if ( pNotification )
		{
			if ( !Q_strcmp( pNotification->GetUnlocalizedText(), "TF_HasNewItems") )
			{
				return;
			}
		}
	}

	// Only provide a notification if there are items to pick up.
	if ( TFInventoryManager()->GetNumItemPickedUpItems() == 0 )
		return;

	TFInventoryManager()->GetLocalTFInventory()->NotifyHasNewItems();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdateNotifications()
{
	return;

	int iNumNotifications = NotificationQueue_GetNumMainMenuNotifications();

	wchar_t wszNumber[16]=L"";
	V_swprintf_safe( wszNumber, L"%i", iNumNotifications );
	wchar_t wszText[1024]=L"";
	g_pVGuiLocalize->ConstructString_safe( wszText, g_pVGuiLocalize->Find( "#MMenu_Notifications_Show" ), 1, wszNumber );

	m_pNotificationsPanel->SetDialogVariable( "notititle", wszText );

	bool bHasNotifications = iNumNotifications != 0;

	if ( m_pNotificationsShowPanel )
	{
		SetNotificationsButtonVisible( bHasNotifications );

		bool bBlinkNotifications = bHasNotifications && m_pNotificationsShowPanel->IsVisible();
		if ( bBlinkNotifications )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pNotificationsShowPanel, "HasNotificationsBlink" );
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( m_pNotificationsShowPanel, "HasNotificationsBlinkStop" );
		}
	}

	if ( !bHasNotifications )
	{
		SetNotificationsButtonVisible( false );
		SetNotificationsPanelVisible( false );
	}

	AdjustNotificationsPanelHeight();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetNotificationsButtonVisible( bool bVisible )
{
	return;

	if ( bVisible && ( m_pNotificationsPanel && m_pNotificationsPanel->IsVisible() ) )
		return;

	if ( m_pNotificationsShowPanel )
	{
		// Show the notifications show panel button if we have new notifications.
		m_pNotificationsShowPanel->SetVisible( bVisible );

		// Set the notification count variable.
		if ( m_pNotificationsShowPanel )
		{
			m_pNotificationsShowPanel->SetDialogVariable( "noticount", NotificationQueue_GetNumMainMenuNotifications() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::SetNotificationsPanelVisible( bool bVisible )
{
	return;

	if ( m_pNotificationsPanel )
	{
		bool bHasNotifications = NotificationQueue_GetNumMainMenuNotifications() != 0;

		if ( bHasNotifications )
		{
			UpdateNotifications();
		}

		m_pNotificationsPanel->SetVisible( bVisible );

		if ( bVisible )
		{
			m_pNotificationsScroller->InvalidateLayout();
			m_pNotificationsScroller->GetScrollbar()->InvalidateLayout();
			m_pNotificationsScroller->GetScrollbar()->SetValue( 0 );

			m_pNotificationsShowPanel->SetVisible( false );

			m_pNotificationsControl->OnTick();
			m_pNotificationsControl->PerformLayout();
			AdjustNotificationsPanelHeight();

			// Faster updating while open.
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
			vgui::ivgui()->AddTickSignal( GetVPanel(), 5 );
		}
		else
		{
			// Clear all notifications.
			if ( bHasNotifications )
			{
				SetNotificationsButtonVisible( true );
			}

			// Slower updating while closed.
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
			vgui::ivgui()->AddTickSignal( GetVPanel(), 250 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::AdjustNotificationsPanelHeight()
{
	return;

	// Fit to our contents, which may change without notifying us.
	int iNotiTall = m_pNotificationsControl->GetTall();
	if ( iNotiTall > m_pNotificationsScroller->GetTall() )
		iNotiTall = m_pNotificationsScroller->GetTall();
	int iTargetTall = YRES(40) + iNotiTall;
	if ( m_pNotificationsPanel->GetTall() != iTargetTall )
		m_pNotificationsPanel->SetTall( iTargetTall );

	// Adjust visibility of the slider buttons and our width, as contents change.
	if ( m_pNotificationsScroller )
	{
		if ( m_pNotificationsScroller->GetScrollbar()->GetSlider() &&
			m_pNotificationsScroller->GetScrollbar()->GetSlider()->IsSliderVisible() )
		{
			m_pNotificationsPanel->SetWide( m_iNotiPanelWide +  m_pNotificationsScroller->GetScrollbar()->GetSlider()->GetWide() );
			m_pNotificationsScroller->GetScrollbar()->SetScrollbarButtonsVisible( true );
		}
		else
		{
			m_pNotificationsPanel->SetWide( m_iNotiPanelWide );
			m_pNotificationsScroller->GetScrollbar()->SetScrollbarButtonsVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::UpdatePromotionalCodes( void )
{
	// should we show the promo codes button?
	vgui::Panel *pPromoCodesButton = FindChildByName( "ShowPromoCodesButton" );
	if ( pPromoCodesButton )
	{
		bool bShouldBeVisible = false;
		if ( steamapicontext && steamapicontext->SteamUser() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			GCSDK::CGCClientSharedObjectCache *pSOCache = GCClientSystem()->GetSOCache( steamID );
			if ( pSOCache )
			{
				GCSDK::CGCClientSharedObjectTypeCache *pTypeCache = pSOCache->FindTypeCache( k_EEconTypeClaimCode );
				bShouldBeVisible = pTypeCache != NULL && pTypeCache->GetCount() != 0;
			}
		}

		// has the player turned off this button?
		if ( !cl_promotional_codes_button_show.GetBool() )
		{
			bShouldBeVisible = false;
		}

		if ( pPromoCodesButton->IsVisible() != bShouldBeVisible )
		{
			pPromoCodesButton->SetVisible( bShouldBeVisible );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CHudMainMenuOverride::IsVisible( void )
{
	/*
	// Only draw whenever the main menu is visible
	if ( GetClientModeTFNormal()->GameUI() && steamapicontext && steamapicontext->SteamFriends() )
		return GetClientModeTFNormal()->GameUI()->IsMainMenuVisible();
	return BaseClass::IsVisible();
	*/
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CExplanationPopup* CHudMainMenuOverride::StartHighlightAnimation( mm_highlight_anims iAnim )
{
	switch( iAnim )
	{
		case MMHA_TUTORIAL:		return ShowDashboardExplanation( "TutorialHighlight" );
		case MMHA_PRACTICE:		return ShowDashboardExplanation( "PracticeHighlight" );
		case MMHA_NEWUSERFORUM:	return ShowDashboardExplanation( "NewUserForumHighlight" );
		case MMHA_OPTIONS:		return ShowDashboardExplanation( "OptionsHighlightPanel" );
		case MMHA_LOADOUT:		return ShowDashboardExplanation( "LoadoutHighlightPanel" );
		case MMHA_STORE:		return ShowDashboardExplanation( "StoreHighlightPanel" );
	}

	Assert( false );
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Make the glows behind the update buttons stop pulsing
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::StopUpdateGlow()
{
	// Dont ever glow again
	if ( cl_mainmenu_updateglow.GetInt() )
	{
		cl_mainmenu_updateglow.SetValue( 0 );
		engine->ClientCmd_Unrestricted( "host_writeconfig" );
	}

	if ( m_pEventPromoContainer )
	{
		EditablePanel* pUpdateBackground = m_pEventPromoContainer->FindControl< EditablePanel >( "Background", true );
		if ( pUpdateBackground )
		{
			g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( pUpdateBackground, "MMenu_UpdateButton_StartGlow" );
			pUpdateBackground->SetControlVisible( "ViewDetailsGlow", false, true );
			pUpdateBackground->SetControlVisible( "ViewWarButtonGlow", false, true );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::OnCommand( const char *command )
{
	C_CTFGameStats::ImmediateWriteInterfaceEvent( "on_command(main_menu_override)", command );

	if ( Q_strnicmp( command, "soundentry", 10 ) == 0 )
	{
		PlaySoundEntry( command + 11 );
		return;
	}
	else if ( !Q_stricmp( command, "opentf2options" ) )
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine opentf2options" );
	}
	else if ( !Q_stricmp( command, "noti_show" ) )
	{
		SetNotificationsPanelVisible( true );
	}
	else if ( !Q_stricmp( command, "noti_hide" ) )
	{
		SetNotificationsPanelVisible( false );
	}
	else if ( !Q_stricmp( command, "notifications_update" ) )
	{
		// force visible if
		if ( NotificationQueue_GetNumMainMenuNotifications() != 0 )
		{
			SetNotificationsButtonVisible( true );
		}
		else
		{
			UpdateNotifications();
		}
	}
	else if ( !Q_stricmp( command, "test_anim" ) )
	{
		InvalidateLayout( true, true );

		StartHighlightAnimation( MMHA_TUTORIAL );
		StartHighlightAnimation( MMHA_PRACTICE );
		StartHighlightAnimation( MMHA_NEWUSERFORUM );
		StartHighlightAnimation( MMHA_OPTIONS );
		StartHighlightAnimation( MMHA_STORE );
		StartHighlightAnimation( MMHA_LOADOUT );
	}
	if ( !Q_stricmp( command, "armory_open" ) )
	{
		GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( "engine open_charinfo_armory" );
	}
	else if ( !Q_stricmp( command, "engine disconnect" ) && engine->IsInGame() && TFGameRules() && ( TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsCompetitiveMode() ) )
	{
		// If we're playing MvM, "New Game" should take us back to MvM matchmaking
		CTFDisconnectConfirmDialog *pDialog = BuildDisconnectConfirmDialog();
		if ( pDialog )
		{
			pDialog->Show();
		}
		return;
	}
	else if ( !Q_stricmp( command, "showpromocodes" ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch ( GetUniverse() )
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStr1024( "http://steamcommunity.com/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64() ) ); break;
			case k_EUniverseBeta:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStr1024( "http://beta.steamcommunity.com/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64() ) ); break;
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( CFmtStr1024( "http://localhost/community/profiles/%llu/promocodes/tf2", steamID.ConvertToUint64() ) ); break;
			}
		}
	}
	else if ( !Q_stricmp( command, "exitreplayeditor" ) )
	{
 #if defined( REPLAY_ENABLED )
		CReplayPerformanceEditorPanel *pEditor = ReplayUI_GetPerformanceEditor();
		if ( !pEditor )
			return;

		pEditor->Exit_ShowDialogs();
#endif // REPLAY_ENABLED
	}
	else if ( FStrEq( "view_update_page", command ) )
	{
		StopUpdateGlow();

		if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->IsOverlayEnabled() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch ( GetUniverse() )
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/meetyourmatch" ); break;
			case k_EUniverseBeta:	// Fall through
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://csham.valvesoftware.com/tf.com/meetyourmatch" ); break;
			}
		}
		else
		{
			OpenStoreStatusDialog( NULL, "#MMenu_OverlayRequired", true, false );
		}
		return;
	}
	else if ( FStrEq( "view_update_comic", command ) )
	{
		if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->IsOverlayEnabled() )
		{
			CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
			switch ( GetUniverse() )
			{
			case k_EUniversePublic: steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/gargoyles_and_gravel" ); break;
			case k_EUniverseBeta:	// Fall through
			case k_EUniverseDev:	steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( "http://www.teamfortress.com/gargoyles_and_gravel" ); break;
			}
		}
		else
		{
			OpenStoreStatusDialog( NULL, "#MMenu_OverlayRequired", true, false );
		}
		return;
	}
	else if ( FStrEq( "OpenMutePlayerDialog", command ) )
	{
		if ( !m_hMutePlayerDialog.Get() )
		{
			VPANEL hPanel = enginevgui->GetPanel( PANEL_GAMEUIDLL );
			vgui::Panel* pPanel = vgui::ipanel()->GetPanel( hPanel, "BaseUI" );

			m_hMutePlayerDialog = vgui::SETUP_PANEL( new CMutePlayerDialog( pPanel ) );
			int x, y, ww, wt, wide, tall;
			vgui::surface()->GetWorkspaceBounds( x, y, ww, wt );
			m_hMutePlayerDialog->GetSize( wide, tall );

			// Center it, keeping requested size
			m_hMutePlayerDialog->SetPos( x + ( ( ww - wide ) / 2 ), y + ( ( wt - tall ) / 2 ) );
		}
		m_hMutePlayerDialog->Activate();
	}
	else
	{
		// Pass it on to GameUI main menu
		if ( GetClientModeTFNormal()->GameUI() )
		{
			GetClientModeTFNormal()->GameUI()->SendMainMenuCommand( command );
			return;
		}
	}

	BaseClass::OnCommand( command );
}

void CHudMainMenuOverride::OnKeyCodePressed( KeyCode code )
{
	if ( code == KEY_XBUTTON_B && engine->IsInGame() )
	{
		OnCommand( "ResumeGame" );
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}

#define REMAP_COMMAND( oldCommand, newCommand ) \
	const char *pszKey##oldCommand = engine->Key_LookupBindingExact(#oldCommand); \
	const char *pszNewKey##oldCommand = engine->Key_LookupBindingExact(#newCommand); \
	if ( pszKey##oldCommand && !pszNewKey##oldCommand ) \
	{ \
		Msg( "Rebinding key %s to new command " #newCommand ".\n", pszKey##oldCommand ); \
		engine->ClientCmd_Unrestricted( VarArgs( "bind \"%s\" \"" #newCommand "\"\n", pszKey##oldCommand ) ); \
	}

//-----------------------------------------------------------------------------
// Purpose: Rebinds any binds for old commands to their new commands.
//-----------------------------------------------------------------------------
void CHudMainMenuOverride::PerformKeyRebindings( void )
{
	REMAP_COMMAND( inspect, +inspect );
	REMAP_COMMAND( taunt, +taunt );
	REMAP_COMMAND( use_action_slot_item, +use_action_slot_item );
	REMAP_COMMAND( use_action_slot_item_server, +use_action_slot_item_server );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainMenuToolTip::PerformLayout()
{
	if ( !ShouldLayout() )
		return;

	_isDirty = false;

	// Resize our text labels to fit.
	int iW = 0;
	int iH = 0;
	for (int i = 0; i < m_pEmbeddedPanel->GetChildCount(); i++)
	{
		vgui::Label *pLabel = dynamic_cast<vgui::Label*>( m_pEmbeddedPanel->GetChild(i) );
		if ( !pLabel )
			continue;

		// Only checking to see if we have any text
		char szTmp[2];
		pLabel->GetText( szTmp, sizeof(szTmp) );
		if ( !szTmp[0] )
			continue;

		pLabel->InvalidateLayout(true);

		int iX, iY;
		pLabel->GetPos( iX, iY );
		iW = MAX( iW, ( pLabel->GetWide() + (iX * 2) ) );

		if ( iH == 0 )
		{
			iH += MAX( iH, pLabel->GetTall() + (iY * 2) );
		}
		else
		{
			iH += MAX( iH, pLabel->GetTall() );
		}
	}
	m_pEmbeddedPanel->SetSize( iW, iH );

	m_pEmbeddedPanel->SetVisible(true);

	PositionWindow( m_pEmbeddedPanel );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainMenuToolTip::HideTooltip()
{
	if ( m_pEmbeddedPanel )
	{
		m_pEmbeddedPanel->SetVisible(false);
	}

	BaseTooltip::HideTooltip();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainMenuToolTip::SetText(const char *pszText)
{
	if ( m_pEmbeddedPanel )
	{
		_isDirty = true;

		if ( pszText && pszText[0] == '#' )
		{
			m_pEmbeddedPanel->SetDialogVariable( "tiptext", g_pVGuiLocalize->Find( pszText ) );
		}
		else
		{
			m_pEmbeddedPanel->SetDialogVariable( "tiptext", pszText );
		}
		m_pEmbeddedPanel->SetDialogVariable( "tipsubtext", "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Reload the .res file
//-----------------------------------------------------------------------------
