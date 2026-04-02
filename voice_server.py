#!/usr/bin/env python3
"""
Pomodoro Chatbot - PC Ses Sunucusu
ESP32'den ham PCM alır → Whisper STT → Ollama LLM → Piper TTS → WAV döner
"""

import os
import io
import wave
import tempfile
import subprocess
import logging
import time
import re
from flask import Flask, request, send_file, jsonify, make_response
import requests
import urllib.parse
from faster_whisper import WhisperModel

# Turkce karakterleri LVGL'in destekledigi ASCII'ye donustur
def tr_to_ascii(text: str) -> str:
    tr_map = {
        "\u015e": "S", "\u015f": "s", "\u011e": "G", "\u011f": "g",
        "\u0130": "I", "\u0131": "i", "\u00d6": "O", "\u00f6": "o",
        "\u00dc": "U", "\u00fc": "u", "\u00c7": "C", "\u00e7": "c",
        "\u00c2": "A", "\u00e2": "a"
    }
    return text.translate(str.maketrans(tr_map))

# ─── Dil Seçimi ──────────────────────────────────────────────────────────────
# "tr" → Türkçe  |  "en" → English
LANGUAGE = "en"
# ─────────────────────────────────────────────────────────────────────────────

# ─── Yapılandırma ────────────────────────────────────────────────────────────
WHISPER_MODEL    = "medium"      # tiny/base/small/medium/large-v2
WHISPER_DEVICE   = "cpu"         # "cpu" or "cuda"
WHISPER_COMPUTE  = "int8"        # int8 (CPU), float16 (GPU)
WHISPER_LANGUAGE = LANGUAGE      # otomatik olarak LANGUAGE'den alınır

OLLAMA_URL       = "http://localhost:11434/api/generate"
OLLAMA_MODEL     = "gemma3:4b"

if LANGUAGE == "tr":
    SYSTEM_PROMPT = (
        "Sen yardımcı ve neşeli bir masaüstü asistansın. Her zaman Türkçe konuşman gerekiyor. "
        "Cevapların kısa ve öz olsun, maksimum 2-3 cümle yaz. "
        "Her zaman gerçek bir cevap ver; sadece 'tamam' veya boş bir şey yazma. "
        "Orijinal Pomodoro tekniğini uyguluyorsun: 25 dakika çalışma, 5 dakika kısa mola ve her 4 çalışmadan sonra 20 dakika uzun mola. "
        "KOMUT ETİKETLERİ KURALLARI - yalnızca açıkça istendiğinde kullan: "
        "KURAL 1: Kullanıcı 'pomodoro başla', 'çalışmayı başlat', 'sayacı koy' derse yanıtının EN SONUNA [CMD:POMO_START] ekle. "
        "KURAL 2: Kullanıcı 'pomodoro dur', 'sayacı durdur', 'çalışmayı bitir' derse yanıtının EN SONUNA [CMD:POMO_STOP] ekle. "
        "Selamlama, soru sorma, sohbet, bilgi isteme gibi durumlar için kesinlikle ETIKET KULLANMA. "
        "YANLIŞ örnek: 'Merhaba nasılsın?' -> 'Pomodoro başlatıyor! [CMD:POMO_START]' (YAPMA!) "
        "DOĞRU örnek: 'Pomodoro başlatır mısın?' -> 'Tabii ki! Başarılı bir çalışma dilerim. [CMD:POMO_START]'"
    )
    # Pomodoro event'leri için LLM prompt'ları
    PROMPT_WORK_DONE          = "Kullanıcının 25 dakikalık çalışma süresi (Pomodoro) bitti. Ona çok kısa ve neşeli bir şekilde 5 dakikalık bir mola vermesini söyle ve tebrik et."
    PROMPT_WORK_DONE_LONG     = "Kullanıcı 4. pomodorosunu bitirdi! Bu harika bir başarı. Şimdi ona 20 dakikalık uzun bir mola vermesini söyle ve çok içten tebrik et."
    PROMPT_BREAK_DONE         = "Kullanıcının 5 dakikalık molası bitti. Ona çok kısa ve enerjik bir şekilde tekrar çalışmaya dönme zamanı olduğunu söyle."
    PROMPT_LONG_BREAK_DONE    = "Kullanıcının 20 dakikalık uzun molası bitti. Harika dinlenmiş olmalı. Şimdi tekrar tam odaklanma ile çalışmaya başlama vaktinin geldiğini söyle."
    # Sensör bağlamı
    SENSOR_CONTEXT_TMPL       = "[SİSTEM NOTU: Odanın şu anki sıcaklığı {temp:.1f}°C, ışık seviyesi %{light:.0f}, ortam gürültüsü {noise:.1f} dB] "
    SENSOR_WORK_CTX           = "[SİSTEM: Kullanıcı ŞU AN pomodoro çalışmasında aktif. Dikkatini dağıtma. YENİ SAYAÇ BAŞLATMA ETİKETİ KULLANMA! Sadece sorusuna yanıt ver.]\n"
    SENSOR_BREAK_CTX          = "[SİSTEM: Kullanıcı şu an pomodoro {state} molasında. YENİ SAYAÇ BAŞLATMA ETİKETİ KULLANMA!]\n"
    # Sensör uyarı prompt'u
    SENSOR_WARN_SYS           = "[SİSTEM: Sen sevimli ve düşünceli bir fiziksel masaüstü asistanısın.]\n"
    SENSOR_WARN_TMPL          = "Kullanıcı çalışıyor fakat ortamı için şu uyarıyı yapman gerekiyor: '{reason}'. Lütfen sadece tek bir cümleyle çok kısa, kibar ve sevimli bir şekilde kullanıcıyı uyar. Çok uzatma, sadece öneride bulun (örneğin: 'Odan çok sıcak, istersen biraz camı açabilirsin.')."
    SENSOR_WARN_TEMP          = "Oda çok sıcak ({temp} derece)"
    SENSOR_WARN_LIGHT         = "Oda fazla karanlık (Işık seviyesi %{light})"
    SENSOR_WARN_NOISE         = "Ortam çok gürültülü ({noise:.1f} dB)"
    # Fallback yanıtları
    FALLBACK_OK               = "Tamamdır."
    FALLBACK_RETRY            = "Anlayamadim, tekrar eder misin?"
    ERROR_NO_AUDIO            = "Ses verisi yok"
    ERROR_NO_SPEECH           = "Konuşma algılanamadı"
else:  # "en"
    SYSTEM_PROMPT = (
        "You are a helpful and cheerful desktop assistant. Always reply in English. "
        "Keep your answers short and concise, maximum 2-3 sentences. "
        "Always give a real answer; never just say 'okay' or leave blank. "
        "You use the original Pomodoro technique: 25 minutes of work, 5-minute short break, and a 20-minute long break after every 4 sessions. "
        "COMMAND TAG RULES - only use when explicitly requested: "
        "RULE 1: If the user says 'start pomodoro', 'start working', 'set the timer', append [CMD:POMO_START] at the very END of your reply. "
        "RULE 2: If the user says 'stop pomodoro', 'stop the timer', 'finish working', append [CMD:POMO_STOP] at the very END of your reply. "
        "Do NOT use tags for greetings, questions, chat, or information requests. "
        "WRONG example: 'Hi, how are you?' -> 'Starting Pomodoro! [CMD:POMO_START]' (DON'T DO THIS!) "
        "CORRECT example: 'Can you start a Pomodoro?' -> 'Sure! Wishing you a productive session. [CMD:POMO_START]'"
    )
    PROMPT_WORK_DONE          = "The user's 25-minute Pomodoro session is over. Tell them briefly and cheerfully to take a 5-minute break and congratulate them."
    PROMPT_WORK_DONE_LONG     = "The user just finished their 4th Pomodoro! That is a great achievement. Tell them to take a 20-minute long break and congratulate them warmly."
    PROMPT_BREAK_DONE         = "The user's 5-minute break is over. Tell them briefly and energetically that it's time to get back to work."
    PROMPT_LONG_BREAK_DONE    = "The user's 20-minute long break is over. They should be well-rested. Tell them it's time to start working again with full focus."
    SENSOR_CONTEXT_TMPL       = "[SYSTEM NOTE: Current room temperature is {temp:.1f}°C, light level is {light:.0f}%, ambient noise is {noise:.1f} dB] "
    SENSOR_WORK_CTX           = "[SYSTEM: User is currently in an active Pomodoro work session. Do not distract them. DO NOT ADD A START TIMER TAG! Just answer their question.]\n"
    SENSOR_BREAK_CTX          = "[SYSTEM: User is currently in a Pomodoro {state} break. DO NOT ADD A START TIMER TAG!]\n"
    SENSOR_WARN_SYS           = "[SYSTEM: You are a friendly and thoughtful physical desktop assistant.]\n"
    SENSOR_WARN_TMPL          = "The user is working but you need to warn them about: '{reason}'. Please warn them in a single, short, polite and friendly sentence. Don't elaborate; just suggest (e.g., 'Your room is quite warm, maybe open a window?')."
    SENSOR_WARN_TEMP          = "The room is too hot ({temp} degrees)"
    SENSOR_WARN_LIGHT         = "The room is too dark (light level {light}%)"
    SENSOR_WARN_NOISE         = "The environment is too noisy ({noise:.1f} dB)"
    FALLBACK_OK               = "Got it."
    FALLBACK_RETRY            = "I couldn't understand, could you repeat that?"
    ERROR_NO_AUDIO            = "No audio data"
    ERROR_NO_SPEECH           = "No speech detected"

BASE_DIR         = os.path.dirname(os.path.abspath(__file__))
PIPER_EXE        = os.path.join(BASE_DIR, "piper.exe")
PIPER_MODEL      = os.path.join(
    BASE_DIR,
    "tr_TR-dfki-medium.onnx" if LANGUAGE == "tr" else "en_GB-alba-medium.onnx"
)

MIC_SAMPLE_RATE  = 16000
MIC_CHANNELS     = 1
MIC_SAMPLE_WIDTH = 2   # 16-bit

SERVER_HOST      = "0.0.0.0"
SERVER_PORT      = 8080
# ─────────────────────────────────────────────────────────────────────────────

# ─── Sensör Durumları ────────────────────────────────────────────────────────
sensor_state = {
    "temperature": 25.0,
    "light": 100.0,
    "noise": 0.0,
    "last_update": 0,
    "last_warning_time": 0
}

# Test amaçlı bazı değerler düşürüldü, gerçek kullanımda daha yüksek değerlere ayarlanabilir
WARNING_COOLDOWN = 60 # 30 dakika
TEMP_THRESHOLD_HIGH = 28.0
LIGHT_THRESHOLD_LOW = 10.0
NOISE_THRESHOLD_HIGH = 50.0
# ─────────────────────────────────────────────────────────────────────────────

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s"
)
log = logging.getLogger(__name__)

app = Flask(__name__)

# Whisper modelini başlangıçta yükle
log.info(f"Whisper modeli yükleniyor: {WHISPER_MODEL}")
whisper = WhisperModel(WHISPER_MODEL, device=WHISPER_DEVICE, compute_type=WHISPER_COMPUTE)
log.info("Whisper modeli yüklendi.")


def pcm_to_wav(pcm: bytes) -> bytes:
    """Ham PCM'i WAV container'ına sarar."""
    buf = io.BytesIO()
    with wave.open(buf, "wb") as wf:
        wf.setnchannels(MIC_CHANNELS)
        wf.setsampwidth(MIC_SAMPLE_WIDTH)
        wf.setframerate(MIC_SAMPLE_RATE)
        wf.writeframes(pcm)
    return buf.getvalue()


def stt(wav_bytes: bytes) -> str:
    """Whisper ile WAV → metin."""
    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        f.write(wav_bytes)
        path = f.name
    try:
        segments, _ = whisper.transcribe(
            path,
            language=WHISPER_LANGUAGE,
            beam_size=5,
            vad_filter=True,
        )
        text = " ".join(s.text.strip() for s in segments).strip()
        log.info(f"STT: {text!r}")
        return text
    finally:
        os.unlink(path)


def llm(prompt: str) -> str:
    """Ollama'ya sorup yanıt al."""
    resp = requests.post(
        OLLAMA_URL,
        json={
            "model": OLLAMA_MODEL,
            "prompt": prompt,
            "system": SYSTEM_PROMPT,
            "stream": False,
        },
        timeout=60,
    )
    resp.raise_for_status()
    text = resp.json().get("response", "").strip()
    log.info(f"LLM: {text[:80]}...")
    return text


def tts(text: str) -> bytes:
    """Piper ile metin → WAV bytes."""
    if not os.path.exists(PIPER_EXE):
        raise FileNotFoundError(f"Piper bulunamadı: {PIPER_EXE}")
    if not os.path.exists(PIPER_MODEL):
        raise FileNotFoundError(f"Piper modeli bulunamadı: {PIPER_MODEL}")
    if not os.path.exists(PIPER_MODEL + ".json"):
        raise FileNotFoundError(f"Piper konfig bulunamadı: {PIPER_MODEL}.json")

    with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as f:
        out_path = f.name
    try:
        subprocess.run(
            [PIPER_EXE, "--model", PIPER_MODEL, "--output_file", out_path],
            input=text.encode("utf-8"),
            capture_output=True,
            check=True,
            cwd=BASE_DIR
        )
        with open(out_path, "rb") as f:
            return f.read()
    except subprocess.CalledProcessError as e:
        log.error(f"Piper hatası: {e.stderr.decode()}")
        raise
    finally:
        if os.path.exists(out_path):
            os.unlink(out_path)


# ─── Endpoint'ler ─────────────────────────────────────────────────────────────

@app.route("/health", methods=["GET"])
def health():
    return jsonify({"status": "ok", "model": OLLAMA_MODEL})


@app.route("/transcribe", methods=["POST"])
def transcribe():
    """
    ESP32'den ham PCM alır (16-bit, 16kHz, mono).
    WAV ses yanıtı döner.
    """
    log.info(f"Yeni istek: {len(request.data)} bayt alındı.")
    try:
        event = request.headers.get("X-Event", "")
        
        if event == "POMO_WORK_DONE":
            text = PROMPT_WORK_DONE
            log.info("Event geldi: POMO_WORK_DONE")
            reply = llm(text)
        elif event == "POMO_WORK_DONE_FOR_LONG_BREAK":
            text = PROMPT_WORK_DONE_LONG
            log.info("Event geldi: POMO_WORK_DONE_FOR_LONG_BREAK")
            reply = llm(text)
        elif event == "POMO_BREAK_DONE":
            text = PROMPT_BREAK_DONE
            log.info("Event geldi: POMO_BREAK_DONE")
            reply = llm(text)
        elif event == "POMO_LONG_BREAK_DONE":
            text = PROMPT_LONG_BREAK_DONE
            log.info("Event geldi: POMO_LONG_BREAK_DONE")
            reply = llm(text)
        else:
            if not request.data:
                log.warning("İstek boş geldi.")
                return jsonify({"error": ERROR_NO_AUDIO}), 400

            wav  = pcm_to_wav(request.data)
            text = stt(wav)

            if not text:
                return jsonify({"error": ERROR_NO_SPEECH}), 422

            # Sensör verisi güncelse bağlam ekle
            context = ""
            if time.time() - sensor_state["last_update"] < 3600:
                context = SENSOR_CONTEXT_TMPL.format(
                    temp=sensor_state['temperature'],
                    light=sensor_state['light'],
                    noise=sensor_state['noise']
                )

            # Prompt'a kullanıcının durumunu da ekleyebiliriz
            state = request.headers.get("X-Pomo-State", "IDLE")
            prompt_context = ""
            if state == "WORK":
                prompt_context = SENSOR_WORK_CTX
            elif state in ["BREAK", "LONG_BREAK"]:
                prompt_context = SENSOR_BREAK_CTX.format(state=state.lower())
            
            final_prompt = prompt_context + context + text
            log.info(f"Transcribe LLM Prompt: {final_prompt}")
            reply = llm(final_prompt)
            
        if not reply:
            return jsonify({"error": "LLM yanıtı boş"}), 502

        action = None
        if "[CMD:POMO_START]" in reply:
            action = "POMO_START"
            reply = reply.replace("[CMD:POMO_START]", "").strip()
        elif "[CMD:POMO_STOP]" in reply:
            action = "POMO_STOP"
            reply = reply.replace("[CMD:POMO_STOP]", "").strip()
            
        # Eğer sadece kod tag'ı geldiyse ve metin kalmadıysa varsayılan sesli yanıt oluştur
        if not reply:
            reply = FALLBACK_OK

        # LLM'in uydurdugu TUM [CMD:...] etiketlerini TTS'e gitmeden temizle (regex)
        reply = re.sub(r'\[CMD:[^\]]*\]', '', reply).strip()
        if not reply:
            reply = FALLBACK_RETRY
            
        # tts WAV binary döndürür
        wav_audio = tts(reply)

        # WAV header'ını atlayıp sadece Data chunk'ını (Raw PCM) çıkartalım
        buf = io.BytesIO(wav_audio)
        with wave.open(buf, 'rb') as wf:
            raw_pcm = wf.readframes(wf.getnframes())

        response = make_response(send_file(
            io.BytesIO(raw_pcm),
            mimetype="application/octet-stream",
            as_attachment=True,
            download_name="response.pcm"
        ))
        
        # Eklemeler: Soru ve cevabı URL Encoded header üzerinden ESP32'ye gönder
        try:
            transcript_encoded = urllib.parse.quote(tr_to_ascii(text))
            reply_encoded = urllib.parse.quote(tr_to_ascii(reply))
            response.headers["X-Transcript-URL"] = transcript_encoded
            response.headers["X-Answer-URL"] = reply_encoded
        except Exception as e:
            log.warning(f"Header encode hatasi: {e}")

        if action:
            response.headers["X-Action"] = action
            log.info(f"X-Action başlığı eklendi: {action}")
            
        return response

    except Exception as e:
        log.exception("Pipeline hatası")
        return jsonify({"error": str(e)}), 500


@app.route("/sensor_update", methods=["POST"])
def sensor_update():
    global sensor_state
    try:
        data = request.get_json()
        if not data:
            return "", 204
            
        temp = data.get("temperature", sensor_state["temperature"])
        light = data.get("light", sensor_state["light"])
        noise = data.get("noise", sensor_state["noise"])
        
        sensor_state["temperature"] = temp
        sensor_state["light"] = light
        sensor_state["noise"] = noise
        sensor_state["last_update"] = time.time()
        
        log.info(f"Sensör Güncellemesi: Sıcaklık={temp}C, Işık={light}%, Gürültü={noise:.1f}dB")
        
        # Proaktif uyarı kontrolü
        now = time.time()
        if now - sensor_state["last_warning_time"] > WARNING_COOLDOWN:
            warning_reason = None
            if temp > TEMP_THRESHOLD_HIGH:
                warning_reason = SENSOR_WARN_TEMP.format(temp=temp)
            elif light < LIGHT_THRESHOLD_LOW:
                warning_reason = SENSOR_WARN_LIGHT.format(light=light)
            elif noise > NOISE_THRESHOLD_HIGH:
                warning_reason = SENSOR_WARN_NOISE.format(noise=noise)
                
            if warning_reason:
                sensor_state["last_warning_time"] = now
                log.info(f"Proaktif uyarı tetiklendi: {warning_reason}")
                
                # LLM'e uyarı metni hazırlat
                prompt = SENSOR_WARN_TMPL.format(reason=warning_reason)
                full_prompt = SENSOR_WARN_SYS + prompt
                reply = llm(full_prompt)
                
                # TTS
                wav_audio = tts(reply)
                
                # PCM Çıkar
                buf = io.BytesIO(wav_audio)
                with wave.open(buf, 'rb') as wf:
                    raw_pcm = wf.readframes(wf.getnframes())
                    
                return send_file(
                    io.BytesIO(raw_pcm),
                    mimetype="application/octet-stream",
                    as_attachment=True,
                    download_name="warning.pcm"
                )
                
        # Uyarılık bir durum yoksa
        return "", 204
        
    except Exception as e:
        log.exception("Sensor update hatası")
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    log.info(f"Sunucu başlatılıyor: http://{SERVER_HOST}:{SERVER_PORT}")
    app.run(host=SERVER_HOST, port=SERVER_PORT, debug=False, threaded=True)
