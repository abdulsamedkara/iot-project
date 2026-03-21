# Faz 1 ve Faz 4: Ses Pipeline ve Pomodoro Asistanı Uygulama Planı

ESP32-S3 tabanlı masaüstü chatbot'un ilk fazında tam ses döngüsü kurulacak:
**Mikrofon → STT → Ollama LLM → TTS → Hoparlör**

Hem PC tarafı (Python sunucu) hem ESP32 firmware'i yazılacaktır. Faz 4 ile beraber sunucu tarafına bir Pomodoro Durum Makinesi (State Machine) eklenecektir.

---

## Mimari

```
ESP32-S3                         PC (yerel)
─────────                        ─────────────────────────────────────────
[INMP441] → I2S → kayıt buf  ──POST /transcribe──▶ Whisper STT
                                                          │
[Buton]   → GPIO → hold/rel                    [Pomodoro State Machine] ↔ Ollama LLM
                                                          │
[MAX98357] ← I2S ← audio buf ◀─GET /audio───────── Piper TTS
```

**İletişim:** HTTP üzerinden. Ses verisi raw PCM (16-bit, 16kHz, mono).

---

## Proposed Changes

### PC Sunucu

#### [MODIFY] [voice_server.py](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/voice_server.py)
Flask tabanlı HTTP sunucu. Uç noktaları (Endpoints) güncellenecek:
- `POST /transcribe` — Gelen sesi metne çevirir, Pomodoro durumunu kontrol eder, LLM'e bağlamı ileterek cevap üretir, TTS ile sese çevirir.
- **[NEW] Pomodoro State Machine (Durum Makinesi)** — Kullanıcının komutlarına ("Pomodoro başlat", "Mola ver") göre arka planda 25/5 dk sayaçları çalıştırır.
- `GET /health` — canlılık kontrolü

Bağımlılıklar: `flask`, `openai-whisper`, `requests` (Ollama için), `piper-tts`

---

### ESP32-S3 Firmware

Proje dizini: `c:\Users\Mert\Desktop\pomodoro-chatbot\firmware\`

#### [NEW] CMakeLists.txt + idf_component.yml
ESP-IDF proje yapısı

#### [NEW] config.h
WiFi SSID/şifre, PC sunucu IP:port, GPIO pin tanımları (buton, I2S)

#### [NEW] i2s_mic.c / i2s_mic.h
INMP441 I2S mikrofon sürücüsü (16kHz, 16-bit, mono)

#### [NEW] i2s_player.c / i2s_player.h
MAX98357A I2S hoparlör sürücüsü

#### [NEW] voice_client.c / voice_client.h
HTTP client — WAV/PCM gönderip cevap ses verisini alır

#### [NEW] main.c
Ana görev: Buton basılı tut → kayıt, bırak → gönder → oynat akışı

---

## Donanım Pin Planı (Varsayılan)

| Bileşen   | Sinyal | GPIO |
|-----------|--------|------|
| INMP441   | SCK    | 14   |
| INMP441   | WS     | 12   |
| INMP441   | SD     | 11   |
| MAX98357A | BCK    | 16   |
| MAX98357A | WS/LRC | 17  |
| MAX98357A | DIN    | 15   |
| Buton     | GPIO   | 0    |

> Pinler `config.h` içinde kolayca değiştirilebilir.

---

## Verification Plan

### Automated Tests
1. PC sunucuyu başlat: `python voice_server.py`
2. `curl -X GET http://localhost:5000/health` → 200 OK
3. Bir WAV dosyası ile test: `curl -X POST --data-binary @test.wav http://localhost:5000/transcribe`

### Manuel Doğrulama
1. ESP32'yi flash'la, seri monitörden WiFi bağlantısını doğrula
2. Butona bas-bırak → seri monitörde transkripsiyon metnini gör
3. Hoparlörden Ollama cevabını sesli duy
