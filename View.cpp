// View.cpp : implementation of the CView class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "View.h"

#include <math.h>

#define VIDEO_CY 250
#define AUDIO_CY 500
#define CBOX_DIST 100
#define ADJUST_CY_START 2
#define ADJUST_CX_START 4

extern __int16 Audio_Buffer[MAX_FRAME * 4096];
extern unsigned __int32 Audio_Video_Length;
extern unsigned __int32 Audio_MP3_Length;
extern unsigned __int32 Audio_Length;

// A function function pretranslate Windows messages for this view class
// The MainFrm initially processes the message and then asks the view to translate the message
BOOL CView::PreTranslateMessage(MSG* pMsg)
{
  LRESULT lResult;
  // Intercept VK_LEFT and VK_RIGHT WM_KEYDOWN messages
  // These keyboard messages are sent to the control on the focus in the client area.
  // Here, the keydown message are sent to the the MainFrm using the CallChain procedure.
  if (pMsg->message == WM_KEYDOWN) {
    if (pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT) {
      CallChain(CHAIN_ID, m_hWnd, pMsg->message, pMsg->wParam, pMsg->lParam, lResult);
      // TRUE means that the message is processed and no further processing is required
      return TRUE;
    }
  }
  // FALSE means that the message is not processed.
  return FALSE;
}

// Process OnCreate Message for this View
// Initialize the variables and Add some static text controls and two check boxes to the CView.
LRESULT CView::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  int   cxChar, cyChar;

  InitVars();

  cxChar = LOWORD(GetDialogBaseUnits());
  cyChar = HIWORD(GetDialogBaseUnits());

  RECT r = { ADJUST_CX_START, cyChar * 0 + ADJUST_CY_START, 95 * cxChar, cyChar * 1 + ADJUST_CY_START };
  m_Text0.Create(m_hWnd, r, L"Click Open and select multiple files using Ctrl and Shift keys: GroupX.mp3, GroupX_0.MTS, GroupX_1.MTS, .....",
    WS_CHILD | WS_VISIBLE | SS_LEFT, 0UL, 0U, 0);

  r = { ADJUST_CX_START, cyChar * 1 + ADJUST_CY_START, 95 * cxChar, cyChar * 2 + ADJUST_CY_START };
  m_Text1.Create(m_hWnd, r, L"Change the starting position using the Left or Right key",
    WS_CHILD | WS_VISIBLE | SS_LEFT, 0UL, 1U, 0);

  r = {ADJUST_CX_START, cyChar * 2 + ADJUST_CY_START, 95 * cxChar, cyChar * 3 + ADJUST_CY_START };
  m_Text2.Create(m_hWnd, r, L"Shift->Left or Right changes the zoom level",
    WS_CHILD | WS_VISIBLE | SS_LEFT, 0UL, 2U, 0);

  r = { ADJUST_CX_START, cyChar * 3 + ADJUST_CY_START, 95 * cxChar, cyChar * 4 + ADJUST_CY_START };
  m_Text3.Create(m_hWnd, r, L"Ctrl-Left or Right shifts the Audio relative to the Video",
    WS_CHILD | WS_VISIBLE | SS_LEFT, 0UL, 3U, 0);

  r = { ADJUST_CX_START, cyChar * 4 + ADJUST_CY_START, 95 * cxChar, cyChar * 5 + ADJUST_CY_START };
  m_Text4.Create(m_hWnd, r, L"You can enable or disable the audio playback with the Video and Audio checkboxes",
    WS_CHILD | WS_VISIBLE | SS_LEFT, 0UL, 4U, 0);

  r = { ADJUST_CX_START, cyChar * 5 + ADJUST_CY_START, 95 * cxChar, cyChar * 6 + ADJUST_CY_START };
  m_Text5.Create(m_hWnd, r, L"Click Save to save SyncX.bat on the directory with the current Start position",
    WS_CHILD | WS_VISIBLE | SS_LEFT, 0UL, 5U, 0);

  r = { cxChar, VIDEO_CY - CBOX_DIST, 13 * cxChar, VIDEO_CY - CBOX_DIST + cyChar};
  m_Btn1.Create(m_hWnd, r, L"Video(MTS)",
    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0UL, 6U, 0);
  m_Btn1.SetCheck(1);

  r = { cxChar, AUDIO_CY - CBOX_DIST, 13 * cxChar, AUDIO_CY - CBOX_DIST + cyChar };
  m_Btn2.Create(m_hWnd, r, L"Audio(MP3)",
    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 0UL, 7U, 0);
  m_Btn2.SetCheck(1);

  return 0;
}

LRESULT CView::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	CPaintDC dc(m_hWnd);
  RECT rect;
	//TODO: Add your drawing code here
  int savedDC = dc.SaveDC();
  int nGroupInt = round(nGroup);
  dc.GetClipBox(&rect);
  dc.SelectStockPen(DC_PEN);
  dc.SetDCPenColor(RGB(192, 192, 192));
  for (int i = 100; i < rect.right; i += 100)
  {
    dc.MoveTo(i, rect.top);
    dc.LineTo(i, rect.bottom);
  }
  dc.SetDCPenColor(RGB(255, 0, 0));
  dc.MoveTo(0, VIDEO_CY);
  for (int i = 0; i < rect.right; i++)
  {
    int min=100000, max=-100000;
    for (int j = 0; j < nGroupInt; j++) {
      //int val = Audio_Video[nBase + i * nGroupInt + j];
      int val = Audio_Buffer[(nBase + i * nGroupInt + j)*2];
      if (val < min) min = val;
      if (val > max) max = val;
    }
    dc.LineTo(i, VIDEO_CY + min/ 150);
    dc.LineTo(i, VIDEO_CY + max/ 150);
  }
  dc.SetDCPenColor(RGB(0, 0, 255));
  dc.MoveTo(0, AUDIO_CY);
  for (int i = 0; i < rect.right; i++)
  {
    int min = 100000, max = -100000;
    for (int j = 0; j < nGroupInt; j++) {
      int val;
      if (nBase + nShift + i * nGroupInt + j >= 0)
       // val = Audio_MP3[nBase + nShift + i * nGroupInt + j];
        val = Audio_Buffer[(nBase + nShift + i * nGroupInt + j) * 2+1];
      else
        val = 0;
      if (val < min) min = val;
      if (val > max) max = val;
    }
    dc.LineTo(i, AUDIO_CY + min / 150);
    dc.LineTo(i, AUDIO_CY + max / 150);
  }
  dc.RestoreDC(savedDC);
  return 0;
}

LRESULT CView::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  LRESULT lResult;
  CallChain(CHAIN_ID, m_hWnd, uMsg, wParam, lParam, lResult);
  return 0;
}

LRESULT CView::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  // If Left arrow (37) or Right arrow (39) is pressed
  // Shift-left (16), Shift-right()
  // Ctrl-left (), Ctrl-right()
  // Alt-left, Alt-right
  switch (wParam) {
  case 16: //bShift = 0;
    break;
  case 17: //bCtrl = 0;
    break;
  default:
    break;
  }
  return 0;
}

LRESULT CView::OnKillFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  //if ((HWND)wParam == m_Btn1.m_hWnd) // || (HWND)wParam == m_Btn2.m_hWnd
  //  SetFocus();
  return 0;
}

