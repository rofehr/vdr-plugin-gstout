# VDR GStreamer Output Plugin - Quick Start

## 🚀 Schnellstart

### 1. Abhängigkeiten installieren

```bash
sudo apt-get install \
  vdr-dev \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-plugins-ugly \
  gstreamer1.0-vaapi
```

### 2. Kompilieren & Installieren

```bash
cd vdr-plugin-gstout
sudo ./install.sh install
```

### 3. VDR starten

```bash
vdr -P gstout
```

## 🎯 Verwendungsbeispiele

### Standard-Konfiguration (Auto-Detection)

```bash
vdr -P gstout
```

### ALSA Audio + X11 Video

```bash
vdr -P "gstout -a alsasink -v xvimagesink"
```

### PulseAudio + VAAPI (Hardware-beschleunigt)

```bash
vdr -P "gstout -a pulsesink -v vaapisink"
```

### Software-Dekodierung

```bash
vdr -P "gstout -D"
```

## 🔧 GStreamer testen

### Audio testen

```bash
# Test-Ton generieren
gst-launch-1.0 audiotestsrc freq=440 ! autoaudiosink

# ALSA testen
gst-launch-1.0 audiotestsrc ! alsasink

# PulseAudio testen
gst-launch-1.0 audiotestsrc ! pulsesink
```

### Video testen

```bash
# Test-Muster generieren
gst-launch-1.0 videotestsrc ! autovideosink

# X11 testen
gst-launch-1.0 videotestsrc ! xvimagesink

# VAAPI testen
gst-launch-1.0 videotestsrc ! vaapisink
```

### VAAPI prüfen

```bash
# VAAPI-Support anzeigen
vainfo

# Ausgabe sollte Profile zeigen:
# VAProfileMPEG2Simple
# VAProfileMPEG2Main
# VAProfileH264Main
# VAProfileH264High
```

## ⚙️ Setup-Menü

Im VDR: **Setup → Plugins → gstout**

Einstellungen:
- **Audio Sink**: Audio-Ausgabemethode wählen
- **Video Sink**: Video-Ausgabemethode wählen
- **Hardware Decoding**: VAAPI aktivieren/deaktivieren
- **Deinterlace**: Deinterlacing aktivieren/deaktivieren
- **Audio Buffer**: Audio-Puffergröße (50-1000 KB)
- **Video Buffer**: Video-Puffergröße (100-2000 KB)

## 📊 SVDRP-Befehle

### Pipeline-Status anzeigen

```bash
svdrpsend PLUG gstout STAT
```

Ausgabe:
```
Audio: PLAYING, Buffer: 45/200 KB
Video: PLAYING, Buffer: 112/200 KB
```

### Pipeline zurücksetzen

```bash
svdrpsend PLUG gstout RSET
```

## 🎬 Pipeline-Architektur

### Audio-Pipeline

```
┌─────────┐   ┌──────────┐   ┌──────────────┐   ┌───────────────┐   ┌──────┐
│ appsrc  │──▶│ decodebin│──▶│ audioconvert │──▶│ audioresample │──▶│ sink │
│ (VDR)   │   │ (Decoder)│   │   (Format)   │   │  (Resample)   │   │(ALSA)│
└─────────┘   └──────────┘   └──────────────┘   └───────────────┘   └──────┘
```

### Video-Pipeline

```
┌─────────┐   ┌──────────┐   ┌─────────────┐   ┌──────────────┐   ┌────────────┐   ┌──────┐
│ appsrc  │──▶│ decodebin│──▶│ deinterlace │──▶│ videoconvert │──▶│ videoscale │──▶│ sink │
│ (VDR)   │   │(Decoder) │   │  (optional) │   │   (Format)   │   │  (Scale)   │   │ (X11)│
└─────────┘   └──────────┘   └─────────────┘   └──────────────┘   └────────────┘   └──────┘
```

## 🐛 Fehlersuche

### Kein Audio

```bash
# 1. GStreamer Audio-Ausgabe testen
gst-launch-1.0 audiotestsrc ! autoaudiosink

# 2. VDR-Log prüfen
journalctl -u vdr | grep gstout

# 3. Anderen Sink versuchen
vdr -P "gstout -a pulsesink"
```

### Kein Video

```bash
# 1. GStreamer Video-Ausgabe testen
gst-launch-1.0 videotestsrc ! autovideosink

# 2. VAAPI prüfen
vainfo
gst-inspect-1.0 vaapi

# 3. Software-Dekodierung versuchen
vdr -P "gstout -D"
```

### Ruckelnde Wiedergabe

1. **Puffer vergrößern** im Setup-Menü
2. **Hardware-Dekodierung aktivieren**
3. **System-Last prüfen**: `top`, `iotop`
4. **GStreamer-Debug aktivieren**:
   ```bash
   export GST_DEBUG=3
   vdr -P gstout
   ```

## 📦 Projektstruktur

```
vdr-plugin-gstout/
│
├── gstout.h/.c          # Haupt-Plugin (VDR-Interface)
├── gstoutput.h/.c       # GStreamer-Engine (Audio/Video)
├── gstsetup.h/.c        # Setup-Menü
│
├── Makefile             # Build-System
├── README.md            # Vollständige Dokumentation
├── QUICKSTART.md        # Diese Datei
├── HISTORY              # Versionshistorie
├── COPYING              # GPL-2 Lizenz
├── install.sh           # Installations-Script
│
└── po/                  # Übersetzungen
    └── de_DE.po        # Deutsche Übersetzung
```

## 🔍 Verfügbare Sinks

### Audio

| Sink | Beschreibung | Verwendung |
|------|--------------|------------|
| `autoaudiosink` | Automatische Erkennung | Desktop/Server |
| `alsasink` | ALSA direkt | Embedded/Low-Level |
| `pulsesink` | PulseAudio | Desktop |
| `osssink` | OSS | Legacy-Systeme |
| `jackaudiosink` | JACK | Pro-Audio |

### Video

| Sink | Beschreibung | Verwendung |
|------|--------------|------------|
| `autovideosink` | Automatische Erkennung | Allgemein |
| `xvimagesink` | X11 XVideo | Desktop (GPU) |
| `ximagesink` | X11 Standard | Desktop (Software) |
| `vaapisink` | VAAPI | Intel/AMD GPU |
| `glimagesink` | OpenGL | Moderne GPU |
| `fbdevsink` | Framebuffer | Embedded/Console |

## ⚡ Performance-Tipps

1. **Hardware-Dekodierung nutzen** (VAAPI bei Intel/AMD)
2. **Passenden Sink wählen**:
   - Audio: PulseAudio für Desktop, ALSA für Embedded
   - Video: VAAPI für Intel-GPUs, XVideo für andere
3. **Puffer optimieren** je nach Inhalt
4. **Deinterlacing deaktivieren** bei progressivem Material
5. **System-Last überwachen** mit `top`

## 📋 Checkliste vor dem Start

- [ ] VDR 2.7.8 installiert
- [ ] GStreamer 1.0 installiert
- [ ] GStreamer-Plugins installiert
- [ ] VAAPI getestet (optional)
- [ ] Audio-Test erfolgreich
- [ ] Video-Test erfolgreich
- [ ] Plugin kompiliert
- [ ] Plugin installiert

## 💡 Nützliche Befehle

```bash
# GStreamer-Version prüfen
gst-inspect-1.0 --version

# Verfügbare Plugins auflisten
gst-inspect-1.0 --plugin

# Spezifischen Sink prüfen
gst-inspect-1.0 alsasink
gst-inspect-1.0 vaapisink

# Pipeline-Graph erstellen (Debug)
export GST_DEBUG_DUMP_DOT_DIR=.
gst-launch-1.0 ... (your pipeline)
# Erzeugt .dot Dateien, konvertieren mit:
dot -Tpng graph.dot -o graph.png

# VDR mit Debug starten
export GST_DEBUG=3
vdr -P gstout 2>&1 | tee vdr.log
```

## 🎓 Weiterführende Links

- [VDR Homepage](https://www.tvdr.de/)
- [GStreamer Dokumentation](https://gstreamer.freedesktop.org/documentation/)
- [VAAPI Wiki](https://wiki.archlinux.org/title/Hardware_video_acceleration)
- [GStreamer Plugin Schreiben](https://gstreamer.freedesktop.org/documentation/plugin-development/)
