<div align="center">
  <h1>🎙️ ESP32 LLM-Supported Pomodoro & Study Environment Assistant</h1>
  <p><strong>A Next-Generation Voice-Controlled IoT Assistant with LLM Integration</strong></p>
</div>

---

## Proje Hakkında (About The Project)

Bu proje, çalışma ortamınızı analiz edebilen, sesli komutlarla kontrol edilebilen ve **ESP32** mikrodenetleyicisi üzerinde koşan akıllı bir masaüstü asistanıdır. Python tabanlı bir sunucu aracılığıyla **Geniş Dil Modelleri (LLM)** ve **Metinden Sese (TTS)** altyapısını kullanarak gerçek zamanlı, akıllı ve konuşkan bir deneyim sunar.

Sadece bir Pomodoro sayacı değil, masanızda sizinle konuşabilen, sorularınıza yapay zeka ile cevap veren tam teşekküllü bir IoT çözümüdür.

### Temel Özellikler (Key Features)

- **🗣️ Gelişmiş Ses Tanıma (Voice Recognition):** ESP32 üzerindeki I2S mikrofon sayesinde kullanıcının sesini yakalayarak sunucuya iletir.
- **🧠 Yapay Zeka Entegrasyonu (LLM):** Gelen sesli komutlar, yerel LLM sunucusu (Ollama vb.) tarafından işlenerek anlamlı metinlere dökülür.
- **🔊 Cihaz Üzerinde Sesli Yanıt (TTS):** Üretilen metinsel yanıtlar Piper TTS kullanılarak yüksek kaliteli sese dönüştürülür ve ESP32 İ2S hoparlörü ile sesli olarak kullanıcıya okunur.
- **⏱️ Akıllı Pomodoro Zamanlayıcı:** Verimli çalışmak için cihaza entegre odaklanma ve mola sayacı.
- **📊 Gerçek Zamanlı Ekran Modülü:** Pomodoro süreleri, sistem durumları ve analiz sonuçları cihazın ekranında anlık olarak görselleştirilir.

---

## Mimari ve Teknolojiler (Architecture & Technologies)

- **Donanım:** ESP32, I2S Mikrofon (örn. INMP441), I2S Ses Amplifikatörü (örn. MAX98357A), LCD Ekran.
- **Gömülü Yazılım (Embedded):** C, ESP-IDF (v5.x), FreeRTOS.
- **Sunucu ve Yapay Zeka (Backend):** Python 3.10+, Piper TTS modülü, ONNX Runtime (`*.ort`), Local LLM.
- **Haberleşme:** ESP32 ve Python Sunucusu arasında HTTP/WebSocket.

---

## Kurulum ve Çalıştırma (Getting Started)

Projenin kendi bilgisayarınızda ve ESP32 donanımınızda çalışması için aşağıdaki adımları izleyin.

### 1. Python Ses Sunucusu (Voice Server) Kurulumu

Bu sunucu, LLM modeli ile etkileşime girmeyi ve TTS işlemlerini yönetmeyi sağlar.

```bash
# Gerekli bağımlılıkları yükleyin
pip install -r requirements.txt

# Sunucuyu başlatın
python voice_server.py
```

*(Not: Git deposu boyutunu aşmamak için `.gitignore` içerisinde kısıtlanmış olan `piper.exe` ve `*.ort` gibi büyük model dosyalarının bu işlemden önce proje dizininde yer aldığından emin olun.)*

### 2. ESP32 Kurulumu ve Gömülmesi (Firmware Build & Flash)

Bilgisayarınızda **ESP-IDF v5.x** ortamının kurulu ve aktif olduğunu varsayıyoruz.

```bash
# ESP-IDF ortamını aktif edin (Örn: get_idf)
get_idf

# Projeyi derleyin
idf.py build

# ESP32'ye kodu yükleyin ve seri monitörü başlatın 
# (COMX yerine kendi USB portunuzu yazın, örn: COM3 veya /dev/ttyUSB0)
idf.py -p COMX flash monitor
```

---

## Geliştiriciler (Developers & Contributors)

Bu uygulama, **Assoc. Prof. Yıldıran Yılmaz** danışmanlığında, dönem projesi kapsamında aşağıda bilgileri yer alan geliştiriciler tarafından hazırlanmıştır:

- **Mert Abdullahoğlu**

- **Abdül Samed Kara**

---
