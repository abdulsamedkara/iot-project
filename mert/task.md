# ESP32-S3 Pomodoro Chatbot - Görev Listesi

## Faz 1: Ses Pipeline (STT → Ollama → TTS)

### PC Sunucu ([voice_server.py](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/voice_server.py))
- [x] HTTP sunucu iskeleti (Flask)
- [x] `/transcribe` endpoint'i (PCM → STT → LLM → TTS → WAV)
- [x] faster-whisper STT entegrasyonu (medium model)
- [x] Ollama LLM API entegrasyonu (gemma3:4b)
- [x] Piper TTS entegrasyonu
- [x] `/health` endpoint'i

### ESP32-S3 Firmware (ESP-IDF)
- [x] Proje yapısı, [CMakeLists.txt](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/firmware/CMakeLists.txt), [sdkconfig.defaults](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/firmware/sdkconfig.defaults)
- [x] [config.h](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/firmware/main/config.h) - WiFi, sunucu IP/port, pin tanımları
- [x] INMP441 I2S mikrofon sürücüsü (`i2s_mic.c/h`)
- [x] MAX98357A I2S hoparlör sürücüsü (`i2s_player.c/h`)
- [x] Buton GPIO (basılı tut → kayıt, bırak → gönder)
- [x] HTTP client (`voice_client.c/h`)
- [x] [main.c](file:///c:/Users/Mert/Desktop/pomodoro-chatbot/firmware/main/main.c) - WiFi + kayıt döngüsü + WAV ayrıştırma + oynatma

## Faz 2: Sensör Entegrasyonu (Tamamlandı)
- [x] DS18B20 sıcaklık sensörü okuma (GPIO 8)
- [x] LDR ışık sensörü okuma (GPIO 5 - ADC1_CH4)
- [x] Sensör verilerini Ollama prompt'una ekleme ve Proaktif Uyarılar

## Faz 2.1: Arka Plan Gürültü Ölçümü (Tamamlandı)
- [x] Olay döngüsünde INMP441 mikrofonundan kısa saniyelik okumalar yaparak RMS/Ortalama gürültü hesaplama
- [x] Gürültü verisini `/sensor_update` endpointi ile sunucuya yollama
- [x] Sunucuda gürültü verisini alıp Ollama için bağlama ekleme ve yüksek gürültü uyarısı yapma

## Faz 3: ILI9341 TFT Ekran
- [ ] SPI/TFT sürücü entegrasyonu (esp_lcd)
- [ ] LVGL ile UI tasarımı (Pomodoro Sayacı, ChatBot durumu)
- [ ] Durum göstergeleri (Düşünüyor, Dinliyor, vb.)

## Faz 4: Pomodoro Zamanlayıcı ve Asistan Mantığı
- [x] 25 dk Çalışma / 5 dk Mola vb. durum makinesi (State Machine)
- [x] Zamanlayıcının ESP32 üzerinde donanımsal takibi
- [x] Süre bitimlerinde sesli bildirim (Sunucudan TTS isteği)
- [x] Sesli komutla Pomodoro başlatma (LLM Intent Parsing)
- [ ] TFT ekranda (Faz 3) kalan pomodoro süresinin görsel olarak (Sayaç) gösterilmesi
