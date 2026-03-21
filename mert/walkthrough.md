# Faz 1 Tamamlandı — Ses Pipeline

## Oluşturulan Dosyalar

```
pomodoro-chatbot/
├── voice_server.py          ← PC ses sunucusu
├── requirements.txt
└── firmware/
    ├── CMakeLists.txt
    ├── sdkconfig.defaults   ← Octal PSRAM ayarları
    └── main/
        ├── CMakeLists.txt
        ├── config.h         ← WiFi / IP / Pin tanımları
        ├── i2s_mic.c/h      ← INMP441 sürücüsü
        ├── i2s_player.c/h   ← MAX98357A sürücüsü
        ├── voice_client.c/h ← HTTP client
        └── main.c           ← Ana uygulama
```

## Kurulum Adımları

### 1. PC Tarafı

```bash
cd pomodoro-chatbot
pip install -r requirements.txt
# Piper'ı indirin ve PATH'e ekleyin
# Türkçe modeli indirin: tr_TR-dfki-medium.onnx
```

[voice_server.py](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/voice_server.py) içinde `PIPER_MODEL` yolunu güncelleyin.

### 2. Firmware

[firmware/main/config.h](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/firmware/main/config.h) içinde **mutlaka** güncelleyin:
- `WIFI_SSID` / `WIFI_PASSWORD`
- `SERVER_HOST` → PC'nizin yerel IP'si

```bash
cd firmware
idf.py set-target esp32s3
idf.py build flash monitor
```

## Çalıştırma

**1. Önce PC sunucuyu başlatın:**
```bash
python voice_server.py
```

**2. ESP32'yi besleyin** — seri monitörde WiFi IP'sini görün.

**3. Butona basılı tutun → konuşun → bırakın** — hoparlörden Gemma3'ün yanıtı gelir.

## Veri Akışı

```
[INMP441] → I2S → PSRAM buf (16kHz PCM)
    → HTTP POST /transcribe (raw PCM)
        → faster-whisper STT
        → Ollama gemma3:4b
        → Piper TTS
    ← HTTP 200 WAV
[MAX98357A] ← I2S ← WAV PCM (22050Hz)
```
