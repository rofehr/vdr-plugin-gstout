/*
 * gstosd.c: OSD provider for GStreamer output
 */

#include "gstosd.h"
#include <vdr/tools.h>
#include <string.h>

// --- cGstOsd ---------------------------------------------------------------

cGstOsd::cGstOsd(int Left, int Top, uint Level, cGstOsdProvider *Provider)
:cOsd(Left, Top, Level)
{
  provider = Provider;
  bitmap = NULL;
  dirty = false;
}

cGstOsd::~cGstOsd()
{
  cMutexLock lock(&mutex);
  delete bitmap;
  
  if (provider)
    provider->ClearOsdBuffer();
}

eOsdError cGstOsd::CanHandleAreas(const tArea *Areas, int NumAreas)
{
  // We can handle any area configuration
  return oeOk;
}

eOsdError cGstOsd::SetAreas(const tArea *Areas, int NumAreas)
{
  cMutexLock lock(&mutex);
  
  if (NumAreas > 0) {
    // Calculate total OSD dimensions
    int maxWidth = 0;
    int maxHeight = 0;
    
    for (int i = 0; i < NumAreas; i++) {
      int right = Areas[i].x1 + Areas[i].Width();
      int bottom = Areas[i].y1 + Areas[i].Height();
      if (right > maxWidth)
        maxWidth = right;
      if (bottom > maxHeight)
        maxHeight = bottom;
    }
    
    // Create bitmap for OSD
    delete bitmap;
    bitmap = new cBitmap(maxWidth, maxHeight, 32); // 32-bit ARGB
    if (!bitmap)
      return oeOutOfMemory;
    
    bitmap->Clean();
    dirty = true;
    
    dsyslog("gstout: OSD areas set: %dx%d", maxWidth, maxHeight);
  }
  
  return cOsd::SetAreas(Areas, NumAreas);
}

void cGstOsd::SaveRegion(int x1, int y1, int x2, int y2)
{
  // Not implemented for now
}

void cGstOsd::RestoreRegion(void)
{
  // Not implemented for now
}

eOsdError cGstOsd::SetPalette(const cPalette &Palette, int Area)
{
  // Not needed for true-color OSD
  return oeOk;
}

void cGstOsd::DrawPixel(int x, int y, tColor Color)
{
  cMutexLock lock(&mutex);
  
  if (bitmap) {
    bitmap->DrawPixel(cPoint(x, y), Color);
    dirty = true;
  }
}

void cGstOsd::DrawBitmap(int x, int y, const cBitmap &Bitmap, tColor ColorFg, tColor ColorBg, bool ReplacePalette, bool Overlay)
{
  cMutexLock lock(&mutex);
  
  if (bitmap) {
    bitmap->DrawBitmap(cPoint(x, y), Bitmap, ColorFg, ColorBg, ReplacePalette, Overlay);
    dirty = true;
  }
}

void cGstOsd::DrawText(int x, int y, const char *s, tColor ColorFg, tColor ColorBg, const cFont *Font, int Width, int Height, int Alignment)
{
  cMutexLock lock(&mutex);
  
  if (bitmap && s && Font) {
    bitmap->DrawText(cPoint(x, y), s, ColorFg, ColorBg, Font, Width, Height, Alignment);
    dirty = true;
  }
}

void cGstOsd::DrawRectangle(int x1, int y1, int x2, int y2, tColor Color)
{
  cMutexLock lock(&mutex);
  
  if (bitmap) {
    bitmap->DrawRectangle(cRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1), Color);
    dirty = true;
  }
}

void cGstOsd::DrawEllipse(int x1, int y1, int x2, int y2, tColor Color, int Quadrants)
{
  cMutexLock lock(&mutex);
  
  if (bitmap) {
    bitmap->DrawEllipse(cRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1), Color, Quadrants);
    dirty = true;
  }
}

void cGstOsd::DrawSlope(int x1, int y1, int x2, int y2, tColor Color, int Type)
{
  cMutexLock lock(&mutex);
  
  if (bitmap) {
    bitmap->DrawSlope(cRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1), Color, Type);
    dirty = true;
  }
}

void cGstOsd::Flush(void)
{
  cMutexLock lock(&mutex);
  
  if (bitmap && dirty && provider) {
    RenderBitmap();
    dirty = false;
  }
}

void cGstOsd::RenderBitmap(void)
{
  if (!bitmap)
    return;
  
  int width = bitmap->Width();
  int height = bitmap->Height();
  
  // Allocate ARGB buffer
  int stride = width * 4; // 4 bytes per pixel (ARGB)
  uint8_t *data = (uint8_t *)malloc(stride * height);
  if (!data) {
    esyslog("gstout: Failed to allocate OSD buffer");
    return;
  }
  
  // Convert bitmap to ARGB
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      tColor color = bitmap->GetColor(x, y);
      int offset = (y * stride) + (x * 4);
      
      // Extract ARGB components
      uint8_t a = (color >> 24) & 0xFF;
      uint8_t r = (color >> 16) & 0xFF;
      uint8_t g = (color >> 8) & 0xFF;
      uint8_t b = color & 0xFF;
      
      // Store in buffer (ARGB format)
      data[offset + 0] = a;
      data[offset + 1] = r;
      data[offset + 2] = g;
      data[offset + 3] = b;
    }
  }
  
  // Send to provider
  provider->UpdateOsdBuffer(data, width, height, stride);
  
  free(data);
}

bool cGstOsd::GetOsdData(uint8_t **data, int *width, int *height, int *stride)
{
  cMutexLock lock(&mutex);
  
  if (!bitmap || !dirty)
    return false;
  
  *width = bitmap->Width();
  *height = bitmap->Height();
  *stride = (*width) * 4;
  
  // Allocate and fill buffer (same as RenderBitmap)
  *data = (uint8_t *)malloc((*stride) * (*height));
  if (!*data)
    return false;
  
  for (int y = 0; y < *height; y++) {
    for (int x = 0; x < *width; x++) {
      tColor color = bitmap->GetColor(x, y);
      int offset = (y * (*stride)) + (x * 4);
      
      (*data)[offset + 0] = (color >> 24) & 0xFF; // A
      (*data)[offset + 1] = (color >> 16) & 0xFF; // R
      (*data)[offset + 2] = (color >> 8) & 0xFF;  // G
      (*data)[offset + 3] = color & 0xFF;         // B
    }
  }
  
  return true;
}

// --- cGstOsdProvider -------------------------------------------------------

cGstOsdProvider::cGstOsdProvider(void)
{
  osd = NULL;
  overlayElement = NULL;
  osdBuffer = NULL;
  osdWidth = 0;
  osdHeight = 0;
  osdStride = 0;
  osdActive = false;
}

cGstOsdProvider::~cGstOsdProvider()
{
  cMutexLock lock(&mutex);
  delete osd;
  free(osdBuffer);
}

cOsd *cGstOsdProvider::CreateOsd(int Left, int Top, uint Level)
{
  cMutexLock lock(&mutex);
  
  if (osd) {
    esyslog("gstout: OSD already exists");
    return NULL;
  }
  
  osd = new cGstOsd(Left, Top, Level, this);
  isyslog("gstout: OSD created at %d,%d level %d", Left, Top, Level);
  
  return osd;
}

void cGstOsdProvider::UpdateOsdBuffer(uint8_t *data, int width, int height, int stride)
{
  cMutexLock lock(&mutex);
  
  // Free old buffer
  free(osdBuffer);
  
  // Allocate new buffer
  int size = stride * height;
  osdBuffer = (uint8_t *)malloc(size);
  if (!osdBuffer) {
    esyslog("gstout: Failed to allocate OSD buffer");
    return;
  }
  
  // Copy data
  memcpy(osdBuffer, data, size);
  
  osdWidth = width;
  osdHeight = height;
  osdStride = stride;
  osdActive = true;
  
  dsyslog("gstout: OSD buffer updated: %dx%d", width, height);
}

void cGstOsdProvider::ClearOsdBuffer(void)
{
  cMutexLock lock(&mutex);
  
  free(osdBuffer);
  osdBuffer = NULL;
  osdWidth = 0;
  osdHeight = 0;
  osdStride = 0;
  osdActive = false;
  
  dsyslog("gstout: OSD buffer cleared");
}

void cGstOsdProvider::ApplyOsdOverlay(GstBuffer *buffer)
{
  cMutexLock lock(&mutex);
  
  if (!osdActive || !osdBuffer || !buffer)
    return;
  
  // Map video buffer
  GstMapInfo map;
  if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
    esyslog("gstout: Failed to map video buffer for OSD overlay");
    return;
  }
  
  // Get video info from caps
  GstCaps *caps = gst_buffer_get_caps(buffer);
  if (!caps) {
    gst_buffer_unmap(buffer, &map);
    return;
  }
  
  GstStructure *structure = gst_caps_get_structure(caps, 0);
  int videoWidth, videoHeight;
  gst_structure_get_int(structure, "width", &videoWidth);
  gst_structure_get_int(structure, "height", &videoHeight);
  gst_caps_unref(caps);
  
  // Simple alpha blending (assumes ARGB format for both)
  // This is a simplified version - proper implementation would handle
  // different video formats and scaling
  
  int minWidth = (osdWidth < videoWidth) ? osdWidth : videoWidth;
  int minHeight = (osdHeight < videoHeight) ? osdHeight : videoHeight;
  
  for (int y = 0; y < minHeight; y++) {
    for (int x = 0; x < minWidth; x++) {
      int osdOffset = (y * osdStride) + (x * 4);
      int videoOffset = (y * videoWidth * 4) + (x * 4);
      
      uint8_t osdA = osdBuffer[osdOffset + 0];
      
      if (osdA > 0) {
        // Simple alpha blending
        uint8_t osdR = osdBuffer[osdOffset + 1];
        uint8_t osdG = osdBuffer[osdOffset + 2];
        uint8_t osdB = osdBuffer[osdOffset + 3];
        
        uint8_t videoR = map.data[videoOffset + 1];
        uint8_t videoG = map.data[videoOffset + 2];
        uint8_t videoB = map.data[videoOffset + 3];
        
        float alpha = osdA / 255.0f;
        
        map.data[videoOffset + 1] = (uint8_t)(osdR * alpha + videoR * (1.0f - alpha));
        map.data[videoOffset + 2] = (uint8_t)(osdG * alpha + videoG * (1.0f - alpha));
        map.data[videoOffset + 3] = (uint8_t)(osdB * alpha + videoB * (1.0f - alpha));
      }
    }
  }
  
  gst_buffer_unmap(buffer, &map);
}
