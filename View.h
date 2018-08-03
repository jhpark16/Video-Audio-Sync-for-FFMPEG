// View.h : interface of the CView class
//
// Author: Jungho Park
// Date: May 2018
// Class definition for CView from Document/View architecture
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "SDL2/SDL.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <SDL2/SDL.h>
#ifdef __cplusplus
};
#endif
#endif

class CView : public CWindowImpl<CView>, public CDynamicChain
{
public:
	DECLARE_WND_CLASS(NULL)

	BOOL PreTranslateMessage(MSG* pMsg);

  HWND  hwndButton[2];
  CStatic m_Text0;
  CStatic m_Text1;
  CStatic m_Text2;
  CStatic m_Text3;
  CStatic m_Text4;
  CStatic m_Text5;
  CButton m_Btn1;
  CButton m_Btn2;
  int nBase;
  int nShift; 
  double nGroup;
  bool bLeft, bRight, bContinue;

  // CDynamicChain allows CallChain(CHAIN_ID, xx) , public CDynamicChain
  // This view class receives all the Windows message targeted to the client area.
  // If CView cannot handle the message, use CallChain to send the message to CMainFrm.
  // Since CView cannot update the Toolbar or Statusbar, any operations updating these 
  // should be sent to CMainFrm.

	BEGIN_MSG_MAP(CView)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
 //   MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
 // MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
  END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

  LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
  LRESULT OnKillFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

  void InitVars()
  {
    nBase = 0;
    nShift = 0;
    nGroup = 1024.0;
    bContinue = false;
  }

  void UpdateAudioCheck()
  {
    bLeft = m_Btn1.GetCheck();
    bRight = m_Btn2.GetCheck();
    bContinue = true;
  }
};

#define CHAIN_ID 1521
#define MAX_FRAME 18000 // It is about 10 minutes
#define AUDIO_CHANNEL_LAYOUT AV_CH_LAYOUT_STEREO // AV_CH_LAYOUT_STEREO // AV_CH_LAYOUT_MONO 
#if AUDIO_CHANNEL_LAYOUT == AV_CH_LAYOUT_STEREO
#define AUDIO_DATA_UNIT 4
#else
#define AUDIO_DATA_UNIT 2
#endif

#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#define AUDIO_OUT_SAMPLE_SIZE 4096
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_IMPORT_LEN 600 // 600 seconds

//Output PCM
//#define OUTPUT_PCM 1
//Use SDL
//#define USE_SDL 1
//#define SHOW_OUTPUT 1


