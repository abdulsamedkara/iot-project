<div align="center">
  <h1>🎙️ ESP32 LLM-Supported Pomodoro & Study Environment Assistant</h1>
  <p><strong>A Next-Generation Voice-Controlled IoT Assistant with LLM Integration</strong></p>
  <p>
    <a href="#english">🇺🇸 English</a> | <a href="#turkce">🇹🇷 Türkçe</a>
  </p>
</div>

---

<a id="english"></a>
## 🇺🇸 English

### 📖 About The Project
This project is a smart desktop assistant running on the **ESP32** microcontroller, capable of analyzing your study environment and responding to voice commands. It provides a real-time, intelligent, and conversational experience using **Large Language Models (LLM)** and **Text-to-Speech (TTS)** via a Python-based server. It is not just a Pomodoro timer, but a full-fledged IoT solution that talks to you and answers your questions with AI.

### ✨ Key Features
- **🗣️ Advanced Voice Recognition:** Captures user voice via I2S microphone on ESP32 and streams it to the server.
- **🧠 AI Integration (LLM):** Incoming voice commands are processed by a local LLM server (e.g., Ollama) and converted into meaningful text responses.
- **🔊 On-Device Voice Response (TTS):** Generated text responses are converted to high-quality audio using Piper TTS and played back through the ESP32 I2S speaker.
- **⏱️ Smart Pomodoro Timer:** Integrated focus and break timer for productive work.
- **📊 Real-Time Display Module:** Visualizes Pomodoro times, system status, and analysis results in real-time on the device display.

### 🛠️ Architecture & Technologies
- **Hardware:** ESP32, I2S Microphone (INMP441), I2S Audio Amplifier (MAX98357A), LCD Display (ILI9341).
- **Embedded:** C, ESP-IDF (v5.x), FreeRTOS.
- **Server & AI (Backend):** Python 3.10+, Piper TTS, ONNX Runtime, Ollama (Local LLM).
- **Communication:** HTTP over Wi-Fi between ESP32 and Python Server.

### 🖼️ Circuit Diagram
Below is the circuit diagram showing the hardware components and connections of the project. This diagram was prepared using **Fritzing**.

![Circuit Diagram](assets/pomodoro%20chatbot.png)

---

### 🚀 Getting Started

**Important Note:** Before building the project, you must open `main/config.h` and replace the placeholder values (`YOUR_WIFI_SSID`, `YOUR_WIFI_PASSWORD`, `YOUR_SERVER_IP`) with your actual Wi-Fi credentials and the local IP address of your host PC.

#### 1. Ollama Setup
Ollama is recommended for the AI engine (LLM).
1. Download and install Ollama from [Ollama.com](https://ollama.com).
2. Pull the required model via terminal:
   ```bash
   ollama run gemma3:4b
   ```

#### 2. Python Voice Server Setup
This server manages LLM interaction and handles TTS tasks.
```bash
# Install required dependencies
pip install -r requirements.txt

# Start the server
python voice_server.py
```

#### 3. Models & Dependencies
For the TTS feature to work, some files must be manually placed in the root directory:
- **Piper TTS (Windows):** Download `piper.exe` and DLLs from [Piper GitHub Releases](https://github.com/rhasspy/piper/releases).
- **Models (ONNX):** Download `tr_TR-dfki-medium` and `en_GB-alba-medium` models from [Piper Voices](https://huggingface.co/rhasspy/piper-voices/tree/main).

> 💡 **Language & Model Configuration:**  
> The overall application language and models can be changed by editing two main files:
> **1. Python Server (`voice_server.py`):**
> - Set `LANGUAGE = "en"` for English and `LANGUAGE = "tr"` for Turkish. This switches server logic, AI prompts, and TTS engines.
> - Change `OLLAMA_MODEL` to use a different local LLM.
> - Change `PIPER_MODEL` to switch your text-to-speech voice `.onnx` file.
> 
> **2. ESP32 Interface (`main/lang.h`):**
> - Open `main/lang.h`.
> - Comment out `#define LANG_TR` and keep `#define LANG_EN` active for English UI, or vice versa for Turkish.

#### 4. ESP32 Setup (Firmware Build & Flash)
```bash
# Activate ESP-IDF environment (e.g., get_idf)
get_idf

# Build the project
idf.py build

# Flash code to ESP32 and open serial monitor
# (Replace COMX with your exact USB port, e.g. COM3 or /dev/ttyUSB0)
idf.py -p COMX flash monitor
```

### 👥 Developers & Contributors
This project was developed within the scope of the **Internet of Things** course at the **Computer Engineering Department of Recep Tayyip Erdogan University**, under the supervision of **Assoc. Prof. Yıldıran Yılmaz**.

- **Mert Abdullahoğlu**
- **Abdül Samed Kara**

---

<br>
<hr>
<br>

<a id="turkce"></a>
## 🇹🇷 Türkçe

### 📖 Proje Hakkında
Bu proje, çalışma ortamınızı analiz edebilen, sesli komutlarla kontrol edilebilen ve **ESP32** mikrodenetleyicisi üzerinde koşan akıllı bir masaüstü asistanıdır. Python tabanlı bir sunucu aracılığıyla **Geniş Dil Modelleri (LLM)** ve **Metinden Sese (TTS)** altyapısını kullanarak gerçek zamanlı, akıllı ve konuşkan bir deneyim sunar. Sadece bir Pomodoro sayacı değil, masanızda sizinle konuşabilen, sorularınıza yapay zeka ile cevap veren tam teşekküllü bir IoT çözümüdür.

### ✨ Temel Özellikler
- **🗣️ Gelişmiş Ses Tanıma:** ESP32 üzerindeki I2S mikrofon sayesinde kullanıcının sesini yakalayarak sunucuya iletir.
- **🧠 Yapay Zeka Entegrasyonu (LLM):** Gelen sesli komutlar, yerel LLM sunucusu (Ollama vb.) tarafından işlenerek anlamlı metinlere dökülür.
- **🔊 Cihaz Üzerinde Sesli Yanıt (TTS):** Üretilen metinsel yanıtlar Piper TTS kullanılarak yüksek kaliteli sese dönüştürülür ve ESP32 I2S hoparlörü ile sesli olarak kullanıcıya okunur.
- **⏱️ Akıllı Pomodoro Zamanlayıcı:** Verimli çalışmak için cihaza entegre odaklanma ve mola sayacı.
- **📊 Gerçek Zamanlı Ekran Modülü:** Pomodoro süreleri, sistem durumları ve analiz sonuçları cihazın ekranında anlık olarak görselleştirilir.

### 🛠️ Mimari ve Teknolojiler
- **Donanım:** ESP32, I2S Mikrofon (INMP441), I2S Ses Amplifikatörü (MAX98357A), LCD Ekran (ILI9341).
- **Gömülü Yazılım:** C, ESP-IDF (v5.x), FreeRTOS.
- **Sunucu ve Yapay Zeka (Backend):** Python 3.10+, Piper TTS, ONNX Runtime, Ollama (Local LLM).
- **Haberleşme:** Wi-Fi üzerinden ESP32 ve Python Sunucusu arasında HTTP.

### 🖼️ Devre Şeması
Aşağıda projenin donanım bileşenlerini ve bağlantılarını gösteren devre şeması yer almaktadır. Bu şema **Fritzing** kullanılarak hazırlanmıştır.

![Circuit Diagram](assets/pomodoro%20chatbot.png)

---

### 🚀 Kurulum ve Çalıştırma

**Önemli Not:** Projeyi derlemeden önce `main/config.h` dosyasını açarak yerleşik olan değerlerin (`YOUR_WIFI_SSID`, `YOUR_WIFI_PASSWORD`, `YOUR_SERVER_IP`) yerine kendi Wi-Fi bilgilerinizi ve sunucu bilgisayarınızın yerel IP adresini girdiğinizden emin olun.

#### 1. Ollama Kurulumu
Projenin zekası (LLM) için Ollama kullanılması önerilir.
1. [Ollama.com](https://ollama.com) adresinden Ollama'yı indirin ve kurun.
2. Terminalden gerekli olan modeli çekin:
   ```bash
   ollama run gemma3:4b
   ```

#### 2. Python Ses Sunucusu Kurulumu
Bu sunucu, LLM modeli ile etkileşime girmeyi ve TTS işlemlerini yönetmeyi sağlar.
```bash
# Gerekli bağımlılıkları yükleyin
pip install -r requirements.txt

# Sunucuyu başlatın
python voice_server.py
```

#### 3. Gerekli Model ve Bağımlılık Dosyaları
Projenin TTS özelliğinin çalışabilmesi için şu dosyaların ana dizine yerleştirilmesi gerekir:
- **Piper TTS (Windows):** [Piper GitHub Releases](https://github.com/rhasspy/piper/releases) üzerinden `piper.exe` ve DLL dosyalarını indirin.
- **Modeller (ONNX):** [Piper Voices](https://huggingface.co/rhasspy/piper-voices/tree/main) üzerinden `tr_TR-dfki-medium` ve `en_GB-alba-medium` modellerini indirin.

> 💡 **Dil ve Model Yapılandırması:**  
> Uygulamanın genel dilini ve arka plan modellerini değiştirmek için yapılandırmanız gereken iki dosya bulunur:
> **1. Python Sunucusu (`voice_server.py`):**
> - `LANGUAGE = "tr"` yapılarak tüm sunucu mantığı, AI davranışları ve TTS sesleri Türkçeye çevrilebilir.
> - `OLLAMA_MODEL` değerini değiştirerek farklı bir LLM modeli kullanabilirsiniz.
> - `PIPER_MODEL` satırından projeye yüklediğiniz farklı bir `.onnx` TTS ses dosyasını seçebilirsiniz.
> 
> **2. ESP32 Arayüzü (`main/lang.h`):**
> - Cihazın LCD ekranındaki metinleri değiştirmek için `main/lang.h` dosyasını açın.
> - Türkçe arayüz için `#define LANG_TR` satırını aktif bırakıp, `#define LANG_EN` satırını yorum satırı yapın. İngilizce için tam tersini uygulayın.

#### 4. ESP32 Kurulumu (Firmware Build & Flash)
```bash
# ESP-IDF ortamını aktif edin (Örn: get_idf)
get_idf

# Projeyi derleyin
idf.py build

# ESP32'ye kodu yükleyin ve seri monitörü başlatın 
# (COMX yerine kendi USB portunuzu yazın, örn: COM3 veya /dev/ttyUSB0)
idf.py -p COMX flash monitor
```

### 👥 Geliştiriciler
Bu proje, **Recep Tayyip Erdoğan Üniversitesi Bilgisayar Mühendisliği** bölümünde **Nesnelerin İnterneti (Internet of Things)** dersi kapsamında, **Doç. Dr. Yıldıran Yılmaz** danışmanlığında hazırlanmıştır.

- **Mert Abdullahoğlu**
- **Abdül Samed Kara**

---
