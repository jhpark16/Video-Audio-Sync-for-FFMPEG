// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "View.h"
#include "MainFrm.h"
#include <string>
#include <codecvt>
#include <algorithm>

static  Uint8  audio_buffer[AUDIO_OUT_SAMPLE_SIZE *AUDIO_DATA_UNIT];
static  Uint32 audio_len;
static  Uint8  *audio_pos;
static  Uint8  *audio_buffer_pos;
static  Uint8  *out_buffer_pos;
__int16 Audio_Buffer[MAX_FRAME * 4096 * 2];
unsigned __int32 Audio_Length = 0;

/* The audio function callback takes the following parameters:
* stream: A pointer to the audio buffer to be filled
* len: The length (in bytes) of the audio buffer
*/
void  fill_audio(void *udata, Uint8 *stream, int len)
{
  //SDL 2.0
  SDL_memset(stream, 0, len);
  if (audio_len == 0)		/*  Only  play  if  we  have  data  left  */
    return;
  len = (len>audio_len ? audio_len : len);	/*  Mix  as  much  data  as  possible  */

  SDL_MixAudio(stream, audio_buffer, len, SDL_MIX_MAXVOLUME);
  audio_len -= len;
}

int CMainFrame::GetAudio(CFileDialogFileList *fileList)
{
  AVFormatContext	*pFormatCtx;
  int				i, audioStream;
  AVCodecContext	*pCodecCtx;
  AVCodec			*pCodec;
  AVPacket		*packet;
  uint8_t			*out_buffer;
  AVFrame			*pFrame;
  SDL_AudioSpec wanted_spec;
  int ret;
  int nOutput;
  int nOutputBytes;
  //uint32_t len = 0;
  int got_picture;
  int index;
  int64_t in_channel_layout;
  struct SwrContext *au_convert_ctx;

  FILE *pFile = NULL;
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
  std::string directory = convert.to_bytes(fileList->directory());
  std::string url_str = directory + convert.to_bytes((*fileList)[idxVideo]);
  std::string url2_str = directory + convert.to_bytes((*fileList)[idxAudio]);
  const char *url = url_str.c_str();
  const char *url2 = url2_str.c_str();
  av_register_all();
  avformat_network_init();
  pFormatCtx = avformat_alloc_context();
  //Open
  if (avformat_open_input(&pFormatCtx, url, NULL, NULL) != 0) {
    printf("Couldn't open input stream.\n");
    return -1;
  }
  // Retrieve stream information
  if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
    printf("Couldn't find stream information.\n");
    return -1;
  }
  // Dump valid information onto standard error
  av_dump_format(pFormatCtx, 0, url, false);

  // Find the first audio stream
  audioStream = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
    if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      audioStream = i;
      break;
    }

  if (audioStream == -1) {
    printf("Didn't find a audio stream.\n");
    return -1;
  }

  // Get a pointer to the codec context for the audio stream
  pCodecCtx = pFormatCtx->streams[audioStream]->codec;

  // Find the decoder for the audio stream
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if (pCodec == NULL) {
    printf("Codec not found.\n");
    return -1;
  }

  // Open codec
  if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
    printf("Could not open codec.\n");
    return -1;
  }

  packet = (AVPacket *)av_malloc(sizeof(AVPacket));
  av_init_packet(packet);

  //Out Audio Param
  uint64_t out_channel_layout = AUDIO_CHANNEL_LAYOUT; //AV_CH_LAYOUT_STEREO; //
                                                      //nb_samples: AAC-1024 MP3-1152
  int out_nb_samples = 1100; //pCodecCtx->frame_size; // 
  AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
  int out_sample_rate = AUDIO_SAMPLE_RATE; //44100; //
  int out_channels = av_get_channel_layout_nb_channels(out_channel_layout);
  //Out Buffer Size
  int out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);

  out_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE * 2);
  pFrame = av_frame_alloc();

  //FIX:Some Codec's Context Information is missing
  in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
  //Swr

  au_convert_ctx = swr_alloc();
  au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
    in_channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);
  swr_init(au_convert_ctx);

  index = 0;
  //Audio_Video_Length=0;
  Audio_Length = 0;

  while (av_read_frame(pFormatCtx, packet) >= 0) {
    if (packet->stream_index == audioStream) {
      ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, packet);
      if (ret < 0) {
        printf("Error in decoding audio frame.\n");
        return -1;
      }
      if (got_picture > 0) {
        nOutput = swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pFrame->data, pFrame->nb_samples);
        //if (index > MAX_FRAME) break; // 4.22 for 12000 (6.5 minutes), 6.04 for 18000 (10 minutes), 8.35 for 24000 (15 minutes), 80000 (42 minutes)
        //if (Audio_Video_Length > AUDIO_SAMPLE_RATE * AUDIO_IMPORT_LEN) break;
        if (Audio_Length > AUDIO_SAMPLE_RATE * AUDIO_IMPORT_LEN) break;
        for (int j = 0; j < nOutput; j++) {
          Audio_Buffer[(Audio_Length + j)*2 + 0] = *(Sint16*)(&out_buffer[j*AUDIO_DATA_UNIT]);
          //Audio_Video[Audio_Video_Length+j] = *(Sint16*)(&out_buffer[j*AUDIO_DATA_UNIT]);
        }
        //Audio_Video_Length += nOutput;
        Audio_Length += nOutput;
        index++;
      }
    }
    //av_free_packet(packet); // deprecated
    av_packet_unref(packet);
  }
  audio_len = audio_buffer_pos - audio_buffer;

  swr_free(&au_convert_ctx);

  //av_free(out_buffer);
  // Close the codec
  avcodec_close(pCodecCtx);
  // Close the video file
  avformat_close_input(&pFormatCtx);

  // Open Audio(MP3) file
  if (avformat_open_input(&pFormatCtx, url2, NULL, NULL) != 0) {
    printf("Couldn't open input stream.\n");
    return -1;
  }
  // Retrieve stream information
  if (avformat_find_stream_info(pFormatCtx, NULL)<0) {
    printf("Couldn't find stream information.\n");
    return -1;
  }
  // Dump valid information onto standard error
  av_dump_format(pFormatCtx, 0, url2, false);

  // Find the first audio stream
  audioStream = -1;
  for (i = 0; i < pFormatCtx->nb_streams; i++)
    if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
      audioStream = i;
      break;
    }

  if (audioStream == -1) {
    printf("Didn't find a audio stream.\n");
    return -1;
  }

  // Get a pointer to the codec context for the audio stream
  pCodecCtx = pFormatCtx->streams[audioStream]->codec;

  // Find the decoder for the audio stream
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if (pCodec == NULL) {
    printf("Codec not found.\n");
    return -1;
  }

  // Open codec
  if (avcodec_open2(pCodecCtx, pCodec, NULL)<0) {
    printf("Could not open codec.\n");
    return -1;
  }

  packet = (AVPacket *)av_malloc(sizeof(AVPacket));
  av_init_packet(packet);

  pFrame = av_frame_alloc();

  //FIX:Some Codec's Context Information is missing
  in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);
  //Swr

  au_convert_ctx = swr_alloc();
  au_convert_ctx = swr_alloc_set_opts(au_convert_ctx, out_channel_layout, out_sample_fmt, out_sample_rate,
    in_channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);
  swr_init(au_convert_ctx);

  index = 0;
  //Audio_MP3_Length = 0;
  Audio_Length = 0;

  while (av_read_frame(pFormatCtx, packet) >= 0) {
    if (packet->stream_index == audioStream) {
      ret = avcodec_decode_audio4(pCodecCtx, pFrame, &got_picture, packet);
      if (ret < 0) {
        printf("Error in decoding audio frame.\n");
        return -1;
      }
      if (got_picture > 0) {
        nOutput = swr_convert(au_convert_ctx, &out_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)pFrame->data, pFrame->nb_samples);
        //if (index > MAX_FRAME) break; // 4.22 for 12000 (6.5 minutes), 6.04 for 18000 (10 minutes), 8.35 for 24000 (15 minutes), 80000 (42 minutes)
        //if (Audio_MP3_Length > AUDIO_SAMPLE_RATE * AUDIO_IMPORT_LEN) break;
        if (Audio_Length > AUDIO_SAMPLE_RATE * AUDIO_IMPORT_LEN) break;
        for (int j = 0; j < nOutput; j++) {
          Audio_Buffer[(Audio_Length + j) * 2 + 1] = *(Sint16*)(&out_buffer[j*AUDIO_DATA_UNIT]);
          //Audio_MP3[Audio_MP3_Length + j] = *(Sint16*)(&out_buffer[j*AUDIO_DATA_UNIT]);
        }
        //Audio_MP3_Length += nOutput;
        Audio_Length += nOutput;
        index++;
      }
    }
    //av_free_packet(packet); // deprecated
    av_packet_unref(packet);
  }
  audio_len = audio_buffer_pos - audio_buffer;

  swr_free(&au_convert_ctx);

#if USE_SDL
  SDL_CloseAudio();//Close SDL
  SDL_Quit();
#endif
  // Close file
#if OUTPUT_PCM
  fclose(pFile);
#endif
  av_free(out_buffer);
  // Close the codec
  avcodec_close(pCodecCtx);
  // Close the video file
  avformat_close_input(&pFormatCtx);

  return 0;
}

// Receive messages from Windows for PreTranslation
// If you need to intercept any messages before it is sent to other windows and controls,
// This is the place you need to do it.

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	return m_view.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
  m_view.bLeft = m_view.m_Btn1.GetCheck();
  m_view.bRight = m_view.m_Btn2.GetCheck();
  UIUpdateToolBar();
  return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	// remove old menu
	SetMenu(NULL);

  m_ToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE,
  ATL_SIMPLE_TOOLBAR_PANE_STYLE);
  CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(m_ToolBar, NULL, TRUE);

	//CreateSimpleStatusBar();
  m_hWndStatusBar = m_wndStatusBar.Create(*this);
  UIAddStatusBar(m_hWndStatusBar);
//  int anPanes[] = { ID_DEFAULT_PANE, ID_PANE_VALUE };
  int anPanes[] = { ID_DEFAULT_PANE, ID_PANE_START, ID_PANE_END, ID_PANE_ZOOM, ID_PANE_DELAY };
  m_wndStatusBar.SetPanes(anPanes, sizeof(anPanes) / sizeof(int), false);
  //m_wndStatusBar.SetPaneWidth(ID_DEFAULT_PANE, 400);
  m_wndStatusBar.SetPaneWidth(ID_PANE_START, 105);
  m_wndStatusBar.SetPaneText(ID_PANE_START, _T("Start 00:00:00.000"));
  m_wndStatusBar.SetPaneWidth(ID_PANE_END, 105);
  m_wndStatusBar.SetPaneText(ID_PANE_END, _T("End 00:01:00.000"));
  m_wndStatusBar.SetPaneWidth(ID_PANE_ZOOM, 60);
  m_wndStatusBar.SetPaneText(ID_PANE_ZOOM, _T("x 1024.0"));
  m_wndStatusBar.SetPaneWidth(ID_PANE_DELAY, 90);
  m_wndStatusBar.SetPaneText(ID_PANE_DELAY, _T("Delay: 0.0"));

	m_hWndClient = m_view.Create(m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
  m_view.SetChainEntry(CHAIN_ID, this);

	UIAddToolBar(m_ToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);

  // register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);
  UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);
	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  CString tStr;
  const int nGroupUnit = 64; // the unit change of the viewport
  const int maxGroup = 16384;
  bool bPanelTimeUpdate = false;
  // If Left arrow (37) or Right arrow (39) is pressed
  // Shift-left (16), Shift-right()
  // Ctrl-left (), Ctrl-right()
  // Alt-left, Alt-right
  bool bShift = (GetKeyState(VK_SHIFT) & 0xff00) !=0;
  bool bCtrl = (GetKeyState(VK_CONTROL) & 0xff00) != 0;
  switch (wParam) {
  case VK_LEFT:
    if (!bShift && !bCtrl) {
      if (m_view.nBase > m_view.nGroup * nGroupUnit) m_view.nBase -= m_view.nGroup * nGroupUnit;
      else m_view.nBase = 0;
      bPanelTimeUpdate = true;
    }
    if (bShift && !bCtrl) {
      if (m_view.nGroup < maxGroup)
        m_view.nGroup *= sqrt(2.0);
      tStr.Format(L"x %.1f", m_view.nGroup);
      m_wndStatusBar.SetPaneText(ID_PANE_ZOOM, tStr);
      bPanelTimeUpdate = true;
    }
    if (!bShift && bCtrl) {
      m_view.nShift += m_view.nGroup;
      tStr.Format(L"Delay: %.4f", (-m_view.nShift/(float)AUDIO_SAMPLE_RATE));
      m_wndStatusBar.SetPaneText(ID_PANE_DELAY, tStr);
    }
    m_view.Invalidate();
    break;
  case VK_RIGHT:
    //if (!m_view.bShift && !m_view.bCtrl && m_view.nBase < Audio_Video_Length - m_view.nGroup * nGroupUnit * 3) {
    if (!bShift && !bCtrl && m_view.nBase < Audio_Length - m_view.nGroup * nGroupUnit * 3) {
        m_view.nBase += m_view.nGroup * nGroupUnit;
      bPanelTimeUpdate = true;
    }
    if (bShift && !bCtrl) {
      if (m_view.nGroup > 4.01) m_view.nGroup /= sqrt(2.0);
      else m_view.nGroup = 4.0;
      tStr.Format(L"x %.1f", m_view.nGroup);
      m_wndStatusBar.SetPaneText(ID_PANE_ZOOM, tStr);
      bPanelTimeUpdate = true;
    }
    if (!bShift && bCtrl) {
      m_view.nShift -= m_view.nGroup;
      tStr.Format(L"Delay: %.4f", (-m_view.nShift/(float)AUDIO_SAMPLE_RATE));
      m_wndStatusBar.SetPaneText(ID_PANE_DELAY, tStr);
    }
    m_view.Invalidate();
    break;
  default:
    break;
  }
  if (bPanelTimeUpdate) UIUpdatePanelTime();
  return 0;
}

void CMainFrame::UIUpdatePanelTime()
{
  CString tStr;
  int nSec, nMin, nHour;
  RECT rect;
  if(m_view.m_hWnd != NULL){
    m_view.GetClientRect(&rect);
    nSec = m_view.nBase;
    nMin = nSec / 60 / AUDIO_SAMPLE_RATE;
    nHour = nMin / 60;
    nSec %= 60 * AUDIO_SAMPLE_RATE;
    nMin %= 60;
    tStr.Format(L"Start: %02d:%02d:%06.3f", nHour, nMin, nSec / (float)AUDIO_SAMPLE_RATE);
    m_wndStatusBar.SetPaneText(ID_PANE_START, tStr);
    nSec = (m_view.nBase + m_view.nGroup*rect.right);
    nMin = nSec / 60 / AUDIO_SAMPLE_RATE;
    nHour = nMin / 60;
    nSec %= 60 * AUDIO_SAMPLE_RATE;
    nMin %= 60;
    tStr.Format(L"End: %02d:%02d:%06.3f", nHour, nMin, nSec / (float)AUDIO_SAMPLE_RATE);
    m_wndStatusBar.SetPaneText(ID_PANE_END, tStr);
  }
}

LRESULT CMainFrame::OnKeyUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
  return 0;
}


LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: add code to initialize document
  for (int i = 0; i < Audio_Length; i++) {
    Audio_Buffer[i * 2] = 0;
    Audio_Buffer[i * 2 + 1] = 0;
  }
  Audio_Length = 0;
  m_view.InitVars();
  m_view.Invalidate();
	return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  int iTmp;
  CString tCstr;
  // TODO: add code to initialize document
  CFileDialog fileDlg(true, _T("tpl"), NULL,
    OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR,
    _T("MP3\0*.mp3;*.mts\0All Files\0*.*\0"));
  if (IDOK == fileDlg.DoModal()) {
    m_fileList = new CFileDialogFileList(fileDlg.m_szFileName);
    idxVideo = -1;
    idxAudio = -1;
    CString tStr = "";
    for (int i = 0; i < m_fileList->num_files(); i++) {
      if ((*m_fileList)[i].Find(L".mp3") > 0) idxAudio = i;
      else {
        if (tStr == "") {
          tStr = (*m_fileList)[i];
          idxVideo = i;
        }
        else {
          if (tStr.CompareNoCase((*m_fileList)[i]) > 0) {
            idxVideo = i;
            tStr = (*m_fileList)[i];
          }
        }
      }
    }
    if (idxVideo == -1 || idxAudio == -1) {
      MessageBox(L"You need to choose at least one Audio(.MP3) and Video(.MTS) files");
    }
    else {
      GetAudio(m_fileList);
      m_view.Invalidate();
      UIUpdatePanelTime();
    }
  }
  return 0;
}

bool CompareCString(CString A, CString B) { return A.CompareNoCase(B)<0; }

LRESULT CMainFrame::OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
  // TODO: add code to initialize document
  if (m_fileList) {
    CString tCstr, videoOutput, videoList;
    std::vector<CString> fileList;
    for (int i = m_fileList->num_files()-1; i >=0 ; i--) {
      tCstr = (*m_fileList)[i];
      if (tCstr.Mid(tCstr.GetLength()-4).CompareNoCase(L".mp3")==0)continue;
      fileList.push_back(tCstr);
    }
    std::sort(fileList.begin(), fileList.end(), CompareCString);
    for (int i = 0; i<fileList.size();i++) {
      if (i == 0) videoList = fileList[i];
      else
        videoList += "|" + fileList[i];
    }
    videoOutput = (*m_fileList)[idxVideo];
    tCstr = m_fileList->directory()+"Sync"+ videoOutput.Mid(5,1)+".bat";
    int idx = videoOutput.ReverseFind('.');
    if (idx > 0) {
      idx--;
      videoOutput.Delete(idx, 1);
      videoOutput.Insert(idx, L"Combined");
      FILE *fp1; ;
      int nSec = round(m_view.nBase/(float)AUDIO_SAMPLE_RATE);
      int nMin = nSec / 60;
      int nHour = nMin / 60;
      nSec %= 60;
      nMin %= 60;
      if ((fp1 = _wfopen(tCstr, L"wt"))!= NULL) {
        tCstr.Format(L"%02d:%02d:%02d", nHour, nMin, nSec);
        fwprintf(fp1, L"REM %s %.4f\n", tCstr, -m_view.nShift / (float)AUDIO_SAMPLE_RATE);
        fwprintf(fp1, L"ffmpeg -y -i \"concat:%s\" -itsoffset %.4f -i \"%s\" -c:v copy -c:a ac3 -map 0:v -map 1:a -map 0:a -ss %s -f mpegts \"%s\"\n",
          videoList, -m_view.nShift / (float)AUDIO_SAMPLE_RATE, (*m_fileList)[idxAudio], tCstr, videoOutput);
        fclose(fp1);
      }
    }
  }
  return 0;
}

LRESULT CMainFrame::OnFileNewWindow(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	::PostThreadMessage(_Module.m_dwMainThreadID, WM_USER, 0, 0L);
	return 0;
}

void CMainFrame::Play() //CView& m_view
{
//  if (Audio_Video_Length > 0) {
    if (Audio_Length > 0) {
    int out_sample_rate = AUDIO_SAMPLE_RATE; //44100; //
    SDL_AudioSpec wanted_spec;
    //SDL_AudioSpec
    SDL_AudioSpec have;
    //Init
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  //
      printf("Could not initialize SDL - %s\n", SDL_GetError());
      return;
    }
    wanted_spec.freq = out_sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.silence = 0;
    wanted_spec.samples = AUDIO_OUT_SAMPLE_SIZE; //out_nb_samples
    wanted_spec.callback = fill_audio;

    if (SDL_OpenAudio(&wanted_spec, &have)<0) {
      printf("can't open audio.\n");
      return;
    }
    audio_buffer_pos = audio_buffer;
    RECT rect;
    m_view.GetClientRect(&rect);
    int nOutput = m_view.nGroup*rect.right;
    out_buffer_pos = (Uint8*)&(Audio_Buffer[m_view.nBase * 2]);
    int curPos = 0;
    int size = AUDIO_OUT_SAMPLE_SIZE;
    audio_len = 0;
    while (nOutput > 0 && m_view.bContinue) {
      while (audio_len > 0)//Wait until finish
        SDL_Delay(1);
      //Set audio buffer (PCM data)
      if (size <= nOutput) {
        for (int i = 0; i < size; i++) {
          if (m_view.bLeft)
            ((Sint16*)audio_buffer)[i * 2] = Audio_Buffer[(m_view.nBase + curPos + i) * 2];
          else
            ((Sint16*)audio_buffer)[i * 2] = 0;
          if (m_view.bRight && m_view.nBase + m_view.nShift + curPos + i >= 0)
              ((Sint16*)audio_buffer)[i * 2 + 1] = Audio_Buffer[(m_view.nBase + m_view.nShift + curPos + i) * 2 + 1];
          else
            ((Sint16*)audio_buffer)[i * 2 + 1] = 0;
        }
        //memcpy(audio_buffer, out_buffer_pos, size);
        out_buffer_pos += size * 4;
        curPos += size;
        nOutput -= size;
        //      audio_buffer_pos = audio_buffer;
        audio_len = size * 4;
      }
      else {
        for (int i = 0; i < nOutput; i++) {
          if (m_view.bLeft)
            ((Sint16*)audio_buffer)[i * 2] = Audio_Buffer[(m_view.nBase + curPos + i) * 2];
          else
            ((Sint16*)audio_buffer)[i * 2] = 0;
          if (m_view.bRight && m_view.nBase + m_view.nShift + curPos + i >= 0)
              ((Sint16*)audio_buffer)[i * 2 + 1] = Audio_Buffer[(m_view.nBase + m_view.nShift + curPos + i) * 2 + 1];
          else
            ((Sint16*)audio_buffer)[i * 2 + 1] = 0;
        }
        //memcpy(audio_buffer, out_buffer_pos, nOutputBytes);
        //      audio_buffer_pos += nOutputBytes;
        audio_len = nOutput * 4;
        nOutput = 0;
      }
      SDL_PauseAudio(0);
    }
    SDL_CloseAudio();//Close SDL
    SDL_Quit();
  }
  m_view.bContinue = false;
  UISetCheck(ID_FILE_PLAY, false);
  UIUpdateToolBar();
  // When you are calling SetFocus, the window must be attached to the calling
  // thread's message queue or SetFocus will return invalid. 
  // To workaround this, use SetForegroundWindow first before calling SetFocus.
  SetForegroundWindow(m_view.m_hWnd);
  m_view.SetFocus();
}

LRESULT CMainFrame::OnFilePlay(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
  if (m_ToolBar.IsButtonChecked(ID_FILE_PLAY)) {
    m_view.bContinue = false;
    UISetCheck(ID_FILE_PLAY, false);
    SetForegroundWindow(m_view.m_hWnd);
    m_view.SetFocus();
  }
  else {
    m_view.UpdateAudioCheck();
    std::thread t1(&CMainFrame::Play, this);
    t1.detach();
    UISetCheck(ID_FILE_PLAY, true);
  }
  return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_BAND_FIRST + 1);	// toolbar is 2nd added band
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainFrame::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
  UIUpdatePanelTime();
  bHandled = false;
  return 0;
}

