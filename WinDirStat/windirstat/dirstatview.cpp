// dirstatview.cpp : Implementation of CDirstatView
//
// see `file_header_text.txt` for licensing & contact info.

#pragma once

#include "stdafx.h"

#ifndef WDS_DIRSTATVIEW_CPP
#define WDS_DIRSTATVIEW_CPP

//encourage inter-procedural optimization (and class-hierarchy analysis!)
#include "ownerdrawnlistcontrol.h"
#include "TreeListControl.h"
#include "item.h"
#include "typeview.h"


#include "options.h"
#include "dirstatview.h"

#include "dirstatdoc.h"
#include "windirstat.h"
#include "mainframe.h"
#include "globalhelpers.h"


namespace {
	const UINT _nIdTreeListControl = 4711;
	}

//BEGIN_MESSAGE_MAP(CMyTreeListControl, CTreeListControl)
//END_MESSAGE_MAP()
//
//CMyTreeListControl::CMyTreeListControl( ) : CTreeListControl( ITEM_ROW_HEIGHT ) { }

/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE( CDirstatView, CView )

BEGIN_MESSAGE_MAP(CDirstatView, CView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_SETFOCUS()
	ON_NOTIFY(LVN_ITEMCHANGED, _nIdTreeListControl, &( CDirstatView::OnLvnItemchanged ) )
	ON_UPDATE_COMMAND_UI(ID_POPUP_TOGGLE, &( CDirstatView::OnUpdatePopupToggle ) )
	ON_COMMAND(ID_POPUP_TOGGLE, &( CDirstatView::OnPopupToggle ) )
END_MESSAGE_MAP()

void CDirstatView::OnSize( UINT nType, INT cx, INT cy ) {
	CView::OnSize( nType, cx, cy );
	if ( IsWindow( m_treeListControl.m_hWnd ) ) {
		CRect rc( 0, 0, cx, cy );
		m_treeListControl.MoveWindow( rc );
		}
	}

void CDirstatView::SetTreeListControlOptions( ) {
	auto Options = GetOptions( );
	       m_treeListControl.ShowGrid            ( Options->m_listGrid             );
	       m_treeListControl.ShowStripes         ( Options->m_listStripes          );
	return m_treeListControl.ShowFullRowSelection( Options->m_listFullRowSelection );
	}

INT CDirstatView::OnCreate( LPCREATESTRUCT lpCreateStruct ) {
	if ( CView::OnCreate( lpCreateStruct ) == -1 ){
		return -1;
		}
	RECT rect = { 0, 0, 0, 0 };
	VERIFY( m_treeListControl.CreateEx( 0, WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS, rect, this, _nIdTreeListControl ) );
	m_treeListControl.AddExtendedStyle( LVS_EX_HEADERDRAGDROP );
	SetTreeListControlOptions( );
	m_treeListControl.InsertColumn( column::COL_NAME,              _T( "Name" ),               LVCFMT_LEFT,  200, column::COL_NAME );
	m_treeListControl.InsertColumn( column::COL_PERCENTAGE,        _T( "Percentage" ),         LVCFMT_RIGHT,  55, column::COL_PERCENTAGE );
	m_treeListControl.InsertColumn( column::COL_SUBTREETOTAL,      _T( "Size" ),               LVCFMT_RIGHT,  90, column::COL_SUBTREETOTAL );
	m_treeListControl.InsertColumn( column::COL_ITEMS,             _T( "Items" ),              LVCFMT_RIGHT,  55, column::COL_ITEMS );
	m_treeListControl.InsertColumn( column::COL_FILES,             _T( "Files" ),              LVCFMT_RIGHT,  55, column::COL_FILES );
  //m_treeListControl.InsertColumn( column::COL_SUBDIRS,           _T( "Subdirs" ),            LVCFMT_RIGHT,  55, column::COL_SUBDIRS );
	m_treeListControl.InsertColumn( column::COL_LASTCHANGE,        _T( "Last Change" ),        LVCFMT_LEFT,  120, column::COL_LASTCHANGE );
	m_treeListControl.InsertColumn( column::COL_ATTRIBUTES,        _T( "Attributes" ),         LVCFMT_LEFT,   50, column::COL_ATTRIBUTES );

	TRACE( _T( "Loading persistent attributes....\r\n" ) );
	m_treeListControl.OnColumnsInserted( );
	return 0;
	}

void CDirstatView::OnUpdatePopupToggle( _In_ CCmdUI* pCmdUI ) {
	pCmdUI->Enable( m_treeListControl.SelectedItemCanToggle( ) );
	}

void CDirstatView::OnPopupToggle( ) {
	m_treeListControl.ToggleSelectedItem( );
	}

void CDirstatView::OnSetFocus( CWnd* pOldWnd ) {
	UNREFERENCED_PARAMETER( pOldWnd );
	m_treeListControl.SetFocus( );
	}

void CDirstatView::OnDestroy( ) {
	CView::OnDestroy();
	}

CDirstatView::CDirstatView( ) : m_treeListControl( ITEM_ROW_HEIGHT, GetDocument( ) ) {// Created by MFC only
	m_treeListControl.SetSorting( column::COL_SUBTREETOTAL, false );
	}

void CDirstatView::SysColorChanged( ) {
	m_treeListControl.SysColorChanged( );
	}


BOOL CDirstatView::OnEraseBkgnd( CDC* pDC ) {
	TRACE( _T( "CDirstatView::OnEraseBkgnd!\r\n" ) );
	UNREFERENCED_PARAMETER( pDC );
	return TRUE;
	}



void CDirstatView::OnLvnItemchanged( NMHDR *pNMHDR, LRESULT *pResult ) {
	const auto pNMLV = reinterpret_cast< LPNMLISTVIEW >( pNMHDR );
	( pResult != NULL ) ? ( *pResult = 0 ) : ASSERT( false );
	if ( ( pNMLV->uChanged & LVIF_STATE ) != 0 ) {
		if ( pNMLV->iItem == -1 ) {
			ASSERT( false ); // mal gucken //'watch times'?
			return;
			}
		// This is not true (don't know why): ASSERT(m_treeListControl.GetItemState(pNMLV->iItem, LVIS_SELECTED) == pNMLV->uNewState);
		const bool selected = ( ( m_treeListControl.GetItemState( pNMLV->iItem, LVIS_SELECTED ) & LVIS_SELECTED ) != 0 );
		const auto item = static_cast< CItemBranch * >( m_treeListControl.GetItem( pNMLV->iItem ) );
		if ( item != NULL ) {
			if ( selected ) {
				const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
				if ( Document != NULL ) {
					ASSERT( item != NULL );
					Document->SetSelection( *item );
					ASSERT( Document == m_pDocument );
					return m_pDocument->UpdateAllViews( this, UpdateAllViews_ENUM::HINT_SELECTIONCHANGED );
					}
				TRACE( _T( "I'm told that the selection has changed in a NULL document?!?? This can't be right.\r\n" ) );
				ASSERT( Document != NULL );
				}
			}
		ASSERT( item != NULL );//We got a NULL item??!? WTF
		}
	}

_Must_inspect_result_ CDirstatDoc* CDirstatView::GetDocument( ) {
	return STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	}


void CDirstatView::OnUpdateHINT_NEWROOT( ) {
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	if ( Document != NULL ) {
		const auto newRootItem = Document->m_rootItem.get( );
		if ( newRootItem != NULL ) {
			m_treeListControl.SetRootItem( newRootItem );
			VERIFY( m_treeListControl.RedrawItems( 0, m_treeListControl.GetItemCount( ) - 1 ) );
			}
		else {
			m_treeListControl.SetRootItem( newRootItem );
			VERIFY( m_treeListControl.RedrawItems( 0, m_treeListControl.GetItemCount( ) - 1 ) );
			}
		}
	ASSERT( Document != NULL );//The document is NULL??!? WTF
	}

void CDirstatView::OnUpdateHINT_SELECTIONCHANGED( ) {
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	if ( Document != NULL ) {
		TRACE( _T( "CDirstatView::OnUpdateHINT_SELECTIONCHANGED\r\n" ) );
		const auto Selection = Document->m_selectedItem;
		ASSERT( Selection != NULL );
		if ( Selection != NULL ) {
			return m_treeListControl.SelectAndShowItem( Selection, false );
			}
		TRACE( _T( "I was told that the selection changed, but found a NULL selection. I can neither select nor show NULL - What would that even mean??\r\n" ) );
		}
	ASSERT( Document != NULL );//The Document has a NULL root item??!?
	}

void CDirstatView::OnUpdateHINT_SHOWNEWSELECTION( ) {
	const auto Document = STATIC_DOWNCAST( CDirstatDoc, m_pDocument );
	if ( Document != NULL ) {
		const auto Selection = Document->m_selectedItem;
		if ( Selection != NULL ) {
			TRACE( _T( "New item selected! item: %s\r\n" ), Selection->GetPath( ).c_str( ) );
			return m_treeListControl.SelectAndShowItem( Selection, true );
			}
		TRACE( _T( "I was told that the selection changed, but found a NULL selection. I can neither select nor show NULL - What would that even mean??\r\n" ) );
		ASSERT( Selection != NULL );
		}
	ASSERT( Document != NULL );//The Document has a NULL root item??!?
	}

void CDirstatView::OnUpdateHINT_LISTSTYLECHANGED( ) {
	TRACE( _T( "List style has changed, redrawing!\r\n" ) );
	const auto Options = GetOptions( );
	m_treeListControl.ShowGrid( Options->m_listGrid );
	m_treeListControl.ShowStripes( Options->m_listStripes );
	m_treeListControl.ShowFullRowSelection( Options->m_listFullRowSelection );
	}

void CDirstatView::OnUpdateHINT_SOMEWORKDONE( ) {
	MSG msg;
	while ( PeekMessageW( &msg, m_treeListControl, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE ) ) {//TODO convert to GetMessage? peek message SPINS and PEGS a SINGLE core at 100%
		if ( msg.message == WM_QUIT ) {
			TRACE( _T( "OnUpdate, case HINT_SOMEWORKDONE: received message to quit!!\r\n" ) );

			PostQuitMessage( static_cast<int>( msg.wParam ) );
			break;
			}
		VERIFY( TranslateMessage( &msg ) );
		DispatchMessageW( &msg );
		}
	}

void CDirstatView::OnUpdate( CView *pSender, LPARAM lHint, CObject *pHint ) {
	switch ( lHint )
	{
		case UpdateAllViews_ENUM::HINT_NEWROOT:
			return OnUpdateHINT_NEWROOT( );

		case UpdateAllViews_ENUM::HINT_SELECTIONCHANGED:
			return OnUpdateHINT_SELECTIONCHANGED( );

		case UpdateAllViews_ENUM::HINT_SHOWNEWSELECTION:
			return OnUpdateHINT_SHOWNEWSELECTION( );

		case UpdateAllViews_ENUM::HINT_REDRAWWINDOW:
			VERIFY( m_treeListControl.RedrawWindow( ) );
			break;

		case UpdateAllViews_ENUM::HINT_ZOOMCHANGED:
			return CView::OnUpdate( pSender, lHint, pHint );

		case UpdateAllViews_ENUM::HINT_LISTSTYLECHANGED:
			return OnUpdateHINT_LISTSTYLECHANGED( );

		case UpdateAllViews_ENUM::HINT_SOMEWORKDONE:
			OnUpdateHINT_SOMEWORKDONE( );
			// fall thru
		case 0:
			CView::OnUpdate( pSender, lHint, pHint );

		default:
			return;
		}
	}


#else

#endif