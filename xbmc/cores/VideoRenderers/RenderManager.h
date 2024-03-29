#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <list>

#if defined (HAS_GL)
  #include "LinuxRendererGL.h"
#elif HAS_GLES == 2
  #include "LinuxRendererGLES.h"
#elif defined(HAS_DX)
  #include "WinRenderer.h"
#elif defined(HAS_SDL)
  #include "LinuxRenderer.h"
#endif

#include "threads/SharedSection.h"
#include "threads/Thread.h"
#include "settings/VideoSettings.h"
#include "OverlayRenderer.h"

class CRenderCapture;
class CDVDClock;

namespace DXVA { class CProcessor; }
namespace VAAPI { class CSurfaceHolder; }
namespace VDPAU { struct CVdpauRenderPicture; }
struct DVDVideoPicture;

#define ERRORBUFFSIZE 30

class CXBMCRenderManager
{
public:
  CXBMCRenderManager();
  ~CXBMCRenderManager();

  // Functions called from the GUI
  void GetVideoRect(CRect &source, CRect &dest) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->GetVideoRect(source, dest); };
  float GetAspectRatio() { CSharedLock lock(m_sharedSection); if (m_pRenderer) return m_pRenderer->GetAspectRatio(); else return 1.0f; };
  void Update(bool bPauseDrawing);
  void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);
  void SetupScreenshot();

  CRenderCapture* AllocRenderCapture();
  void ReleaseRenderCapture(CRenderCapture* capture);
  void Capture(CRenderCapture *capture, unsigned int width, unsigned int height, int flags);
  void ManageCaptures();

  void SetViewMode(int iViewMode) { CSharedLock lock(m_sharedSection); if (m_pRenderer) m_pRenderer->SetViewMode(iViewMode); };

  // Functions called from mplayer
  bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, unsigned int format);
  bool IsConfigured();

  int AddVideoPicture(DVDVideoPicture& picture);

  void FlipPage(volatile bool& bStop, double timestamp = 0.0, int source = -1, EFIELDSYNC sync = FS_NONE, int speed = 0);
  unsigned int PreInit(CDVDClock *pClock);
  void UnInit();
  bool Flush();

  void AddOverlay(CDVDOverlay* o, double pts)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddOverlay(o, pts);
  }

  void AddCleanup(OVERLAY::COverlay* o)
  {
    CSharedLock lock(m_sharedSection);
    m_overlays.AddCleanup(o);
  }

  inline void Reset()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      m_pRenderer->Reset();
  }
  RESOLUTION GetResolution()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetResolution();
    else
      return RES_INVALID;
  }

  float GetMaximumFPS();
  inline bool Paused() { return m_bPauseDrawing; };
  inline bool IsStarted() { return m_bIsStarted;}
  double GetDisplayLatency() { return m_displayLatency; }

  bool Supports(ERENDERFEATURE feature)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->Supports(feature);
    else
      return false;
  }

  bool Supports(EDEINTERLACEMODE method)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->Supports(method);
    else
      return false;
  }

  bool Supports(EINTERLACEMETHOD method)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->Supports(method);
    else
      return false;
  }

  bool Supports(ESCALINGMETHOD method)
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->Supports(method);
    else
      return false;
  }

  EINTERLACEMETHOD AutoInterlaceMethod(EINTERLACEMETHOD mInt)
  {
    CSharedLock lock(m_sharedSection);
    return AutoInterlaceMethodInternal(mInt);
  }

  double GetPresentTime();
  void  WaitPresentTime(double presenttime);

  CStdString GetVSyncState();

  void UpdateResolution();

  unsigned int GetProcessorSize()
  {
    CSharedLock lock(m_sharedSection);
    if (m_pRenderer)
      return m_pRenderer->GetProcessorSize();
    return 0;
  }

#ifdef HAS_GL
  CLinuxRendererGL *m_pRenderer;
#elif HAS_GLES == 2
  CLinuxRendererGLES *m_pRenderer;
#elif defined(HAS_DX)
  CWinRenderer *m_pRenderer;
#elif defined(HAS_SDL)
  CLinuxRenderer *m_pRenderer;
#endif

  void Present();
  void Recover(); // called after resolution switch if something special is needed

  CSharedSection& GetSection() { return m_sharedSection; };

  int WaitForBuffer(volatile bool& bStop);
  void NotifyDisplayFlip();
  bool GetStats(double &sleeptime, double &pts, int &bufferLevel);
  bool HasFrame();
  void EnableBuffering(bool enable);
  void DiscardBuffer();

protected:
  void Render(bool clear, DWORD flags, DWORD alpha);

  void PresentSingle(bool clear, DWORD flags, DWORD alpha);
  void PresentWeave(bool clear, DWORD flags, DWORD alpha);
  void PresentBob(bool clear, DWORD flags, DWORD alpha);
  void PresentBlend(bool clear, DWORD flags, DWORD alpha);

  int  GetNextRenderBufferIndex();
  void FlipRenderBuffer();
  int  FlipFreeBuffer();
  bool HasFreeBuffer();
  void ResetRenderBuffer();
  void PrepareNextRender();

  EINTERLACEMETHOD AutoInterlaceMethodInternal(EINTERLACEMETHOD mInt);

  bool m_bPauseDrawing;   // true if we should pause rendering

  bool m_bIsStarted;
  CSharedSection m_sharedSection;

  bool m_bReconfigured;

  int m_rendermethod;

  enum EPRESENTSTEP
  {
    PRESENT_IDLE     = 0
  , PRESENT_FLIP
  , PRESENT_FRAME
  , PRESENT_FRAME2
  };

  enum EPRESENTMETHOD
  {
    PRESENT_METHOD_SINGLE = 0,
    PRESENT_METHOD_BLEND,
    PRESENT_METHOD_WEAVE,
    PRESENT_METHOD_BOB,
  };

  double m_displayLatency;
  void UpdateDisplayLatency();

  // Render Buffer State Description:
  //
  // Output:      is the buffer about to or having its texture prepared for render (ie from output thread).
  //              Cannot go past the "Displayed" buffer (otherwise we will probably overwrite buffers not yet
  //              displayed or even rendered).
  // Current:     is the current buffer being or having been submitted for render to back buffer.
  //              Cannot go past "Output" buffer (else it would be rendering old output).
  // FlipRequest: is the render buffer that has last been submitted for render AND importantly has had
  //              swap-buffer flip subsequently invoked (thus flip to front buffer is requested for vblank
  //              subsequent to render completion).
  // Displayed:   is the buffer that is now considered to be safely copied from back buffer to front buffer
  //              (we assume that after two swap-buffer flips for the same "Current" render buffer that that
  //              buffer will be safe, but otherwise we consider that only the previous-to-"Current" is guaranteed).
  // Last:        is the last buffer successfully submitted for render to back buffer (purpose: to rollback to in
  //              unexpected case where a texture render fails).

  int m_iCurrentRenderBuffer;
  int m_iNumRenderBuffers;
//  int m_iLastRenderBuffer;
  int m_iFlipRequestRenderBuffer;
  int m_iOutputRenderBuffer;
  int m_iDisplayedRenderBuffer;
  bool m_bAllRenderBuffersDisplayed;
  bool m_bUseBuffering;
  int m_speed;
  CEvent m_flipEvent;

  struct
  {
    double pts;
    EFIELDSYNC presentfield;
    EPRESENTMETHOD presentmethod;
  }m_renderBuffers[5];

  double     m_sleeptime;
  double     m_presentPts;

  double     m_presenttime;
  double     m_presentcorr;
  double     m_presenterr;
  double     m_errorbuff[ERRORBUFFSIZE];
  int        m_errorindex;
  EFIELDSYNC m_presentfield;
  EPRESENTMETHOD m_presentmethod;
  EPRESENTSTEP     m_presentstep;
  int        m_presentsource;
  CEvent     m_presentevent;
  CEvent     m_flushEvent;
  CDVDClock  *m_pClock;
  uint64_t   m_rendertime;
  double     m_swaptime;
  unsigned int m_swapCount;

  OVERLAY::CRenderer m_overlays;

  void RenderCapture(CRenderCapture* capture);
  void RemoveCapture(CRenderCapture* capture);
  CCriticalSection           m_captCritSect;
  std::list<CRenderCapture*> m_captures;
  //set to true when adding something to m_captures, set to false when m_captures is made empty
  //std::list::empty() isn't thread safe, using an extra bool will save a lock per render when no captures are requested
  bool                       m_hasCaptures; 
};

extern CXBMCRenderManager g_renderManager;
