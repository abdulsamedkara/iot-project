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
from flask import Flask, request, send_file, jsonify, make_response
import requests
from faster_whisper import WhisperModel

# ─── Yapılandırma ────────────────────────────────────────────────────────────
WHISPER_MODEL    = "medium"      # tiny/base/small/medium/large-v2
WHISPER_DEVICE   = "cpu"         # "cpu" veya "cuda"
WHISPER_COMPUTE  = "int8"        # int8 (CPU için), float16 (GPU için)
WHISPER_LANGUAGE = "tr"          # "tr" Türkçe, "en" İngilizce

OLLAMA_URL       = "http://localhost:11434/api/generate"
OLLAMA_MODEL     = "gemma3:4b"
SYSTEM_PROMPT    = (
    "Sen yardımcı ve neşeli bir masaüstü asistanısın. Kısa ve öz yanıtlar ver. Türkçe konuş. "
    "DİKKAT - KOMUT ETİKETLERİ: "
    "1) SADECE VE SADECE kullanıcı açıkça 'pomodoro başlat', 'çalışma başlat', 'sayacı kur' gibi bir talepte bulunursa yanıtının en sonuna [CMD:POMO_START] etiketini ekle. "
    "2) SADECE kullanıcı açıkça sayacı 'durdur', 'bitir', 'kapat' derse [CMD:POMO_STOP] etiketini ekle. "
    "Kullanıcı sadece rastgele bir soru soruyorsa (örneğin 'entropi nedir') HİÇBİR ETİKET EKLEME!"
)

BASE_DIR         = os.path.dirname(os.path.abspath(__file__))
PIPER_EXE        = os.path.join(BASE_DIR, "piper.exe")
PIPER_MODEL      = os.path.join(BASE_DIR, "tr_TR-dfki-medium.onnx")

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
            text = "Kullanıcının 25 dakikalık çalışma süresi (Pomodoro) bitti. Ona çok kısa ve neşeli bir şekilde mola vermesini söyle ve tebrik et."
            log.info("Event geldi: POMO_WORK_DONE")
            reply = llm(text)
        elif event == "POMO_BREAK_DONE":
            text = "Kullanıcının 5 dakikalık molası bitti. Ona çok kısa ve enerjik bir şekilde tekrar çalışmaya dönmesini söyle."
            log.info("Event geldi: POMO_BREAK_DONE")
            reply = llm(text)
        else:
            if not request.data:
                log.warning("İstek boş geldi.")
                return jsonify({"error": "Ses verisi yok"}), 400

            wav  = pcm_to_wav(request.data)
            text = stt(wav)

            if not text:
                return jsonify({"error": "Konuşma algılanamadı"}), 422

            # Sensör verisi güncelse bağlam ekle
            context = ""
            if time.time() - sensor_state["last_update"] < 3600:
                context = f"[SİSTEM NOTU: Odanın şu anki sıcaklığı {sensor_state['temperature']:.1f}°C, ışık seviyesi %{sensor_state['light']:.0f}, ortam gürültüsü {sensor_state['noise']:.1f} dB] "

            # Prompt'a kullanıcının durumunu da ekleyebiliriz
            state = request.headers.get("X-Pomo-State", "IDLE")
            prompt_context = ""
            if state == "WORK":
                prompt_context = "[SİSTEM: Kullanıcı ŞU AN pomodoro çalışmasında aktif. Dikkatini dağıtma. YENİ SAYAÇ BAŞLATMA ETİKETİ KULLANMA! Sadece sorusuna yanıt ver.]\n"
            elif state == "BREAK":
                prompt_context = "[SİSTEM: Kullanıcı şu an pomodoro molasında. YENİ SAYAÇ BAŞLATMA ETİKETİ KULLANMA!]\n"
            
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
            reply = "Tamamdır."
            
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
                warning_reason = f"Oda çok sıcak ({temp} derece)"
            elif light < LIGHT_THRESHOLD_LOW:
                warning_reason = f"Oda fazla karanlık (Işık seviyesi %{light})"
            elif noise > NOISE_THRESHOLD_HIGH:
                warning_reason = f"Ortam çok gürültülü ({noise:.1f} dB)"
                
            if warning_reason:
                sensor_state["last_warning_time"] = now
                log.info(f"Proaktif uyarı tetiklendi: {warning_reason}")
                
                # LLM'e uyarı metni hazırlat
                prompt = (
                    f"Kullanıcı çalışıyor fakat ortamı için şu uyarıyı yapman gerekiyor: '{warning_reason}'. "
                    "Lütfen sadece tek bir cümleyle çok kısa, kibar ve sevimli bir şekilde kullanıcıyı uyar. "
                    "Çok uzatma, sadece öneride bulun (örneğin: 'Odan çok sıcak, istersen biraz camı açabilirsin.')."
                )
                
                full_prompt = "[SİSTEM: Sen sevimli ve düşünceli bir fiziksel masaüstü asistanısın.]\n" + prompt
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
