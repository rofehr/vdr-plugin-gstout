# OSD Support in GStreamer Output Plugin

## Overview

The GStreamer output plugin includes a fully integrated OSD provider that renders VDR's on-screen displays (menus, EPG, subtitles, etc.) directly over the video output. This is achieved through alpha-blended overlays using 32-bit ARGB rendering.

## Architecture

### OSD Provider (`cGstOsdProvider`)

The OSD provider is registered with VDR during plugin initialization and handles all OSD creation requests from VDR.

**Key Features:**
- Implements VDR's `cOsdProvider` interface
- Creates `cGstOsd` instances for each OSD window
- Maintains ARGB buffer for OSD content
- Provides buffer to video pipeline for blending

### OSD Implementation (`cGstOsd`)

Each OSD window is an instance of `cGstOsd` which:
- Extends VDR's `cOsd` base class
- Renders to 32-bit ARGB bitmap
- Supports all VDR drawing operations
- Converts VDR color format to ARGB

### OSD Rendering Pipeline

```
VDR Core
  │
  ├─> DrawText() ───┐
  ├─> DrawBitmap()──┤
  ├─> DrawRect() ───┤
  └─> DrawPixel() ──┤
                    │
                    ▼
              cGstOsd::bitmap
                (ARGB)
                    │
              Flush()│
                    ▼
         cGstOsdProvider::osdBuffer
                    │
                    ▼
         cGstVideoOutput Pipeline
                    │
         Alpha Blending
                    │
                    ▼
              Video Output
```

## Drawing Operations

The OSD supports all standard VDR drawing operations:

### Text Rendering
```cpp
DrawText(x, y, text, ColorFg, ColorBg, Font, Width, Height, Alignment);
```
- Anti-aliased text using VDR fonts
- Full Unicode support
- Configurable alignment

### Bitmap Drawing
```cpp
DrawBitmap(x, y, Bitmap, ColorFg, ColorBg, ReplacePalette, Overlay);
```
- Supports VDR bitmap format
- Palette replacement
- Overlay mode for transparency

### Shape Drawing
```cpp
DrawRectangle(x1, y1, x2, y2, Color);
DrawEllipse(x1, y1, x2, y2, Color, Quadrants);
DrawSlope(x1, y1, x2, y2, Color, Type);
```

## Color Format

### VDR tColor Format
VDR uses 32-bit ARGB color format:
```
Bit 31-24: Alpha (transparency)
Bit 23-16: Red
Bit 15-8:  Green
Bit 7-0:   Blue
```

### Alpha Channel
- 0x00 = Fully transparent
- 0xFF = Fully opaque
- Values in between = Semi-transparent

### Example Colors
```cpp
0xFF000000  // Fully opaque black
0x00000000  // Fully transparent
0x80FF0000  // Semi-transparent red
0xFFFFFFFF  // Fully opaque white
```

## Alpha Blending

The plugin performs alpha blending of OSD over video using the standard formula:

```
result = (osd * alpha) + (video * (1 - alpha))
```

Where:
- `osd` = OSD pixel color (RGB)
- `alpha` = OSD alpha channel (0.0 - 1.0)
- `video` = Video pixel color (RGB)
- `result` = Blended output color (RGB)

## Performance Considerations

### Buffer Management
- OSD buffer is only allocated when OSD is active
- Buffer is freed when OSD is closed
- Dirty flag prevents unnecessary updates

### Blending Optimization
- Blending only performed on dirty regions
- Skip pixels with alpha = 0 (fully transparent)
- Use SIMD instructions where available

### Thread Safety
- All OSD operations are mutex-protected
- Video pipeline access is synchronized
- No race conditions between OSD updates and video frames

## Configuration

### Enable/Disable OSD
```bash
# Via command line (future enhancement)
vdr -P "gstout --osd-blend=yes"

# Via setup menu
Setup → Plugins → gstout → OSD Blending
```

### OSD Buffer Size
The OSD buffer size is determined by the OSD area dimensions requested by VDR. Typical sizes:
- SD (720x576): ~1.6 MB
- HD (1920x1080): ~8 MB

## Troubleshooting

### OSD Not Visible

1. **Check OSD Blending Setting**
   ```bash
   # In VDR Setup
   Setup → Plugins → gstout → OSD Blending = Yes
   ```

2. **Verify OSD Creation**
   ```bash
   # Check VDR log
   grep "gstout.*OSD" /var/log/syslog
   
   # Should show:
   # "OSD provider registered"
   # "OSD created at X,Y level L"
   ```

3. **Check Video Format**
   - OSD blending requires ARGB video format
   - Some sinks may not support this format

### OSD Performance Issues

1. **Reduce OSD Area Size**
   - Smaller OSD areas = less blending overhead
   - Use only necessary area dimensions

2. **Disable Anti-Aliasing**
   - Some VDR themes use heavy anti-aliasing
   - Try simpler themes

3. **Hardware Acceleration**
   - Enable hardware decoding to free CPU
   - Use GPU-accelerated video sinks (VAAPI, OpenGL)

### OSD Color Issues

1. **Check Color Format**
   - Ensure 32-bit ARGB is used
   - Some older themes may use palette mode

2. **Verify Alpha Channel**
   - Check that alpha channel is properly set
   - 0xFF = opaque, 0x00 = transparent

## Implementation Details

### Memory Layout

ARGB buffer memory layout:
```
Offset  Content
------  -------
0       Alpha (pixel 0,0)
1       Red   (pixel 0,0)
2       Green (pixel 0,0)
3       Blue  (pixel 0,0)
4       Alpha (pixel 1,0)
5       Red   (pixel 1,0)
...
```

### Stride Calculation
```cpp
stride = width * 4;  // 4 bytes per pixel (ARGB)
offset = (y * stride) + (x * 4);
```

### Buffer Allocation
```cpp
bufferSize = stride * height;
osdBuffer = (uint8_t *)malloc(bufferSize);
```

## Future Enhancements

- [ ] Hardware-accelerated blending via GPU
- [ ] Support for multiple OSD layers
- [ ] Region-based dirty tracking
- [ ] OSD scaling for different resolutions
- [ ] Bitmap caching for frequently used graphics
- [ ] SIMD optimizations for blending
- [ ] Support for OSD animations

## API Reference

### cGstOsdProvider

```cpp
class cGstOsdProvider : public cOsdProvider {
public:
  cGstOsdProvider(void);
  virtual ~cGstOsdProvider();
  
  // Create OSD instance
  virtual cOsd *CreateOsd(int Left, int Top, uint Level);
  
  // Apply OSD overlay to video buffer
  void ApplyOsdOverlay(GstBuffer *buffer);
  
  // Update OSD buffer from cGstOsd
  void UpdateOsdBuffer(uint8_t *data, int width, int height, int stride);
  void ClearOsdBuffer(void);
};
```

### cGstOsd

```cpp
class cGstOsd : public cOsd {
public:
  cGstOsd(int Left, int Top, uint Level, cGstOsdProvider *Provider);
  virtual ~cGstOsd();
  
  // Drawing operations
  virtual void DrawPixel(int x, int y, tColor Color);
  virtual void DrawBitmap(...);
  virtual void DrawText(...);
  virtual void DrawRectangle(...);
  virtual void DrawEllipse(...);
  virtual void Flush(void);
  
  // Get rendered OSD data
  bool GetOsdData(uint8_t **data, int *width, int *height, int *stride);
};
```

## Examples

### Creating OSD Manually

```cpp
cGstOsdProvider *provider = new cGstOsdProvider();
cOsd *osd = provider->CreateOsd(0, 0, 0);

if (osd) {
  tArea area = { 0, 0, 719, 575, 32 };  // 720x576, 32-bit
  osd->SetAreas(&area, 1);
  
  // Draw red rectangle
  osd->DrawRectangle(100, 100, 200, 200, 0xFFFF0000);
  
  // Draw text
  const cFont *font = cFont::GetFont(fontOsd);
  osd->DrawText(100, 250, "Hello VDR!", 0xFFFFFFFF, 0xFF000000, font);
  
  osd->Flush();
}
```

### Custom Alpha Blending

```cpp
void BlendPixel(uint8_t *video, uint8_t *osd) {
  float alpha = osd[0] / 255.0f;
  
  video[1] = (uint8_t)(osd[1] * alpha + video[1] * (1.0f - alpha));  // R
  video[2] = (uint8_t)(osd[2] * alpha + video[2] * (1.0f - alpha));  // G
  video[3] = (uint8_t)(osd[3] * alpha + video[3] * (1.0f - alpha));  // B
}
```

## References

- VDR OSD API: `/usr/include/vdr/osd.h`
- GStreamer Video Overlay: https://gstreamer.freedesktop.org/documentation/video/gstvideooverlay.html
- Alpha Compositing: https://en.wikipedia.org/wiki/Alpha_compositing
