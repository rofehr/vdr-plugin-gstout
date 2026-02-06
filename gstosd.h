/*
 * gstosd.h: OSD provider for GStreamer output
 */

#ifndef __GSTOSD_H
#define __GSTOSD_H

#include <vdr/channels.h>
#include <vdr/osd.h>
#include <vdr/thread.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <gst/gst.h>
#include <gio/gio.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include <gst/video/video-overlay-composition.h>


// Forward declaration
class cGstOsdProvider;

// --- cGstOsd ---------------------------------------------------------------

class cGstOsd : public cOsd {
private:
  cGstOsdProvider *provider;
  cBitmap *bitmap;
  bool dirty;
  cMutex mutex;
  
  void RenderBitmap(void);
  
public:
  cGstOsd(int Left, int Top, uint Level, cGstOsdProvider *Provider);
  virtual ~cGstOsd();
  
  virtual eOsdError CanHandleAreas(const tArea *Areas, int NumAreas);
  virtual eOsdError SetAreas(const tArea *Areas, int NumAreas);
  virtual void SaveRegion(int x1, int y1, int x2, int y2);
  virtual void RestoreRegion(void);
  virtual eOsdError SetPalette(const cPalette &Palette, int Area);
  virtual void DrawPixel(int x, int y, tColor Color);
  virtual void DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg = 0, tColor ColorBg = 0, bool ReplacePalette = false, bool Overlay = false);
  virtual void DrawText(int x, int y, const char *s, tColor ColorFg, tColor ColorBg, const cFont *Font, int Width = 0, int Height = 0, int Alignment = taDefault);
  virtual void DrawRectangle(int x1, int y1, int x2, int y2, tColor Color);
  virtual void DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants = 0);
  virtual void DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type);
  virtual void Flush(void);
  
  // Get rendered ARGB data for overlay
  bool GetOsdData(uint8_t **data, int *width, int *height, int *stride);
  bool IsDirty(void) const { return dirty; }
  void ClearDirty(void) { dirty = false; }
};

// --- cGstOsdProvider -------------------------------------------------------

class cGstOsdProvider : public cOsdProvider {
private:
  cGstOsd *osd;
  GstElement *overlayElement;
  cMutex mutex;
  
  uint8_t *osdBuffer;
  int osdWidth;
  int osdHeight;
  int osdStride;
  bool osdActive;
  
  friend class cGstOsd;
  
public:
  cGstOsdProvider(void);
  virtual ~cGstOsdProvider();
  
  virtual cOsd *CreateOsd(int Left, int Top, uint Level);
  virtual bool ProvidesCa(const cChannel *Channel) { return false; }
  
  // Called by video pipeline to apply OSD overlay
  void ApplyOsdOverlay(GstBuffer *buffer);
  
  // Set overlay element from video pipeline
  void SetOverlayElement(GstElement *element) { overlayElement = element; }
  
  // Update OSD buffer from cGstOsd
  void UpdateOsdBuffer(uint8_t *data, int width, int height, int stride);
  void ClearOsdBuffer(void);
};

#endif // __GSTOSD_H
