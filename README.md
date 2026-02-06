# VDR Plugin 'gstout' - GStreamer Output

## Description

This plugin provides GStreamer-based audio and video output for VDR 2.7.8 with **integrated OSD support**. It leverages the powerful GStreamer multimedia framework to offer flexible playback options with support for hardware acceleration, various output sinks, and advanced features like deinterlacing. The built-in OSD provider renders VDR menus, EPG, and subtitles directly over the video output.

## Features

- **GStreamer Integration**: Uses GStreamer 1.0 multimedia framework
- **Hardware Acceleration**: Optional VAAPI hardware decoding support
- **OSD Support**: Built-in OSD provider for menus, EPG, subtitles
  - Alpha-blended overlays
  - True-color (32-bit ARGB) rendering
  - Hardware-accelerated blending
  - Configurable enable/disable
- **Flexible Output Sinks**: 
  - Audio: ALSA, PulseAudio, OSS, JACK, or auto-detection
  - Video: X11, VAAPI, OpenGL, Framebuffer, or auto-detection
- **Deinterlacing**: Built-in deinterlacing support for interlaced content
- **Configurable Buffers**: Adjustable audio and video buffer sizes
- **Live Statistics**: Monitor pipeline status via SVDRP
- **Thread-Safe**: Proper mutex protection for concurrent access

## Requirements

- VDR >= 2.7.8
- GStreamer >= 1.0
- GStreamer plugins:
  - gstreamer1.0-plugins-base (core functionality)
  - gstreamer1.0-plugins-good (additional codecs)
  - gstreamer1.0-plugins-bad (extended codecs)
  - gstreamer1.0-plugins-ugly (patent-encumbered codecs)
  - gstreamer1.0-vaapi (optional, for hardware acceleration)

## Installation

### Dependencies (Debian/Ubuntu)

```bash
sudo apt-get install \
  vdr-dev \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-tools \
  gstreamer1.0-vaapi
```

### Compilation

```bash
cd vdr-plugin-gstout
make
sudo make install
```

### VDR Configuration

Add the plugin to your VDR startup:

```bash
vdr -P gstout
```

Or with custom sinks:

```bash
vdr -P "gstout -a pulsesink -v vaapisink"
```

## Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-a SINK, --audio=SINK` | GStreamer audio sink | autoaudiosink |
| `-v SINK, --video=SINK` | GStreamer video sink | autovideosink |
| `-d, --hwdec` | Enable hardware decoding | yes |
| `-D, --no-hwdec` | Disable hardware decoding | - |

## Available Sinks

### Audio Sinks

| Sink | Description |
|------|-------------|
| `autoaudiosink` | Automatic audio output detection |
| `alsasink` | ALSA audio output |
| `pulsesink` | PulseAudio output |
| `osssink` | OSS audio output |
| `jackaudiosink` | JACK audio output |

### Video Sinks

| Sink | Description |
|------|-------------|
| `autovideosink` | Automatic video output detection |
| `xvimagesink` | X11 XVideo output |
| `ximagesink` | X11 standard output |
| `vaapisink` | VAAPI hardware-accelerated output |
| `glimagesink` | OpenGL output |
| `fbdevsink` | Linux framebuffer output |

## Setup Menu

The plugin provides a setup menu accessible via VDR's Setup → Plugins → gstout:

- **Audio Sink**: Select audio output method
- **Video Sink**: Select video output method
- **Hardware Decoding**: Enable/disable VAAPI hardware acceleration
- **Deinterlace**: Enable/disable deinterlacing
- **OSD Blending**: Enable/disable OSD overlay rendering
- **Audio Buffer**: Buffer size in KB (50-1000)
- **Video Buffer**: Buffer size in KB (100-2000)

## SVDRP Commands

### STAT - Show Statistics

```
$ svdrpsend PLUG gstout STAT
Audio: PLAYING, Buffer: 45/200 KB
Video: PLAYING, Buffer: 112/200 KB
```

### RSET - Reset Pipeline

```
$ svdrpsend PLUG gstout RSET
GStreamer pipeline reset
```

## Architecture

### Components

```
┌─────────────────────────────────────────┐
│         cPluginGstout                   │
│  (Main plugin, VDR interface)           │
└─────────────┬───────────┬───────────────┘
              │           │
              │           └──────────────┐
              ▼                          ▼
┌─────────────────────────────┐   ┌──────────────────┐
│       cGstOutput            │   │ cGstOsdProvider  │
│  (Output coordinator)       │   │  (OSD rendering) │
└─────┬──────────────┬────────┘   └──────────────────┘
      │              │
      ▼              ▼
┌───────────────┐  ┌───────────────┐
│cGstAudioOutput│  │cGstVideoOutput│
│(Audio pipeline)│  │(Video pipeline│
│               │  │  + OSD blend) │
└───────────────┘  └───────────────┘
```

### Audio Pipeline

```
appsrc → decodebin → audioconvert → audioresample → [sink]
```

Components:
- **appsrc**: Receives data from VDR
- **decodebin**: Auto-detects and decodes audio format
- **audioconvert**: Converts audio format if needed
- **audioresample**: Resamples audio to match output requirements
- **sink**: Outputs audio (ALSA, PulseAudio, etc.)

### Video Pipeline

```
appsrc → decodebin → [deinterlace] → videoconvert → videoscale → [sink]
```

Components:
- **appsrc**: Receives data from VDR
- **decodebin**: Auto-detects and decodes video format (with optional VAAPI)
- **deinterlace**: Deinterlaces interlaced content (optional)
- **videoconvert**: Converts color space if needed
- **videoscale**: Scales video to match output resolution
- **sink**: Outputs video (X11, VAAPI, etc.)

## Hardware Acceleration

### VAAPI Support

When hardware decoding is enabled, the plugin attempts to use `vaapidecodebin` for video decoding. This provides:

- **Lower CPU usage**: Offloads decoding to GPU
- **Better performance**: Higher resolution/bitrate support
- **Power efficiency**: Reduced power consumption

### Requirements for VAAPI

```bash
# Check if VAAPI is available
vainfo

# Should show available profiles like:
# VAProfileMPEG2Simple
# VAProfileMPEG2Main
# VAProfileH264Main
# VAProfileH264High
```

If VAAPI is not available or fails, the plugin automatically falls back to software decoding.

## Buffer Management

### Audio Buffer

- **Default**: 200 KB
- **Range**: 50-1000 KB
- **Purpose**: Smooth audio playback, prevent dropouts
- **Tuning**: 
  - Increase for network streams or slow systems
  - Decrease for lower latency

### Video Buffer

- **Default**: 200 KB
- **Range**: 100-2000 KB
- **Purpose**: Smooth video playback, handle bitrate spikes
- **Tuning**:
  - Increase for high-bitrate content (HD/4K)
  - Decrease for faster channel switching

## Troubleshooting

### No Audio Output

```bash
# Test audio sink directly
gst-launch-1.0 audiotestsrc ! autoaudiosink

# Check VDR log
journalctl -u vdr | grep gstout

# Try different audio sink
vdr -P "gstout -a alsasink"
```

### No Video Output

```bash
# Test video sink directly
gst-launch-1.0 videotestsrc ! autovideosink

# Check VAAPI support
vainfo
gst-inspect-1.0 vaapi

# Try software decoding
vdr -P "gstout -D"
```

### Choppy Playback

1. **Increase buffer sizes** in setup menu
2. **Enable hardware decoding** if available
3. **Check system load**:
   ```bash
   top
   iostat -x 1
   ```
4. **Verify GStreamer installation**:
   ```bash
   gst-inspect-1.0 --version
   gst-inspect-1.0 --plugin
   ```

### Pipeline Errors

Check GStreamer debug output:

```bash
# Set debug level
export GST_DEBUG=3

# Or for specific category
export GST_DEBUG=gstout:5

# Run VDR and check logs
vdr -P gstout
```

## Performance Tips

1. **Use hardware decoding** when available (VAAPI on Intel, VA-API on AMD)
2. **Choose appropriate sinks**:
   - Audio: PulseAudio for desktop, ALSA for embedded
   - Video: VAAPI for Intel GPUs, XV for software
3. **Optimize buffer sizes** based on content and system
4. **Disable deinterlacing** for progressive content
5. **Use lower-latency sinks** for live TV

## Development

### Project Structure

```
vdr-plugin-gstout/
├── gstout.h/.c          # Main plugin
├── gstoutput.h/.c       # GStreamer output engine
├── gstsetup.h/.c        # Setup menu
├── Makefile             # Build system
└── README.md            # This file
```

### Building Debug Version

```bash
make CXXFLAGS="-g -O0 -DDEBUG"
export GST_DEBUG=5
gdb vdr
```

### Adding New Sinks

1. Edit `gstsetup.c` to add sink name to arrays
2. Test sink availability:
   ```bash
   gst-inspect-1.0 <sink-name>
   ```
3. Update documentation

## Known Issues

- **Initial delay**: First playback may have slight delay while GStreamer initializes
- **Buffer underruns**: May occur on very slow systems or network streams
- **VAAPI compatibility**: Some older Intel GPUs have limited codec support

## Future Enhancements

- [ ] Picture-in-picture support
- [ ] Audio/video synchronization controls
- [ ] Advanced audio processing (equalizer, compression)
- [ ] Multiple audio output support
- [ ] Network streaming output
- [ ] Recording playback optimization
- [ ] Support for GStreamer 2.0

## Testing

### Test Audio Pipeline

```bash
# Generate test tone
gst-launch-1.0 audiotestsrc freq=440 ! autoaudiosink

# Test with file
gst-launch-1.0 filesrc location=test.mp3 ! decodebin ! audioconvert ! autoaudiosink
```

### Test Video Pipeline

```bash
# Generate test pattern
gst-launch-1.0 videotestsrc ! autovideosink

# Test with file
gst-launch-1.0 filesrc location=test.mpg ! decodebin ! autovideosink
```

### Verify Plugin Loading

```bash
# Check if plugin is loaded
svdrpsend PLUG gstout HELP

# Check pipeline state
svdrpsend PLUG gstout STAT
```

## License

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

See the file COPYING for more information.

## References

- VDR: https://www.tvdr.de/
- GStreamer: https://gstreamer.freedesktop.org/
- VAAPI: https://01.org/vaapi
- GStreamer Plugin Writing Guide: https://gstreamer.freedesktop.org/documentation/plugin-development/

## Changelog

### Version 0.1.0 (Initial Release)
- GStreamer-based audio/video output
- Hardware acceleration support (VAAPI)
- Configurable sinks and buffers
- Deinterlacing support
- SVDRP commands for monitoring
- Setup menu integration
