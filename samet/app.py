# -*- coding: utf-8 -*-
from flask import Flask, request, Response
import numpy as np
import wave
import os
import uuid
import re
from difflib import SequenceMatcher
from faster_whisper import WhisperModel
import requests
import io
import struct
from piper import PiperVoice

app = Flask(__name__)

def generate_tts_wav(text: str) -> bytes:
    """Piper ile TTS üretir ve WAV formatında byte dizisi döndürür."""
    # Cumleyi biraz yumuşatmak için sonuna nokta ekle, aksi halde bazen kesilebilir.
    if not text.endswith(('.', '!', '?')):
        text += '.'
        
    buf = io.BytesIO()
    with wave.open(buf, 'wb') as wf:
        tts_voice.synthesize_wav(text, wf)
        wf.writeframes(b'\x00' * 22050 * 2) # add 1 second of silence
    return buf.getvalue()

SAVE_DIR = "records"
os.makedirs(SAVE_DIR, exist_ok=True)

print("Whisper model yukleniyor...")
whisper_model = WhisperModel("large-v3", device="cpu", compute_type="int8")
print("Whisper hazir.")

OLLAMA_URL = "http://127.0.0.1:11434/api/generate"
OLLAMA_MODEL = "qwen2.5:14b"

print("TTS modeli yukleniyor...")
tts_voice = PiperVoice.load("./piper_models/tr_TR-dfki-medium.onnx")
print("TTS hazir.")

# ---------------------------------------------------------------------------
# Pomodoro komut algılama sabitleri
# ---------------------------------------------------------------------------
POMODORO_KEYWORDS = [
    "pomodoro", "komodoro", "pomidoro", "pomodorro", "bimodoro",
    "pomodora", "pomodaru", "pomadoro", "commodoro",
]
START_KEYWORDS = [
    "başlat", "başla", "baslat", "basla",
    "başlatır", "başlasın", "baslasın", "baslatir",
    "çalıştır", "calistir", "ac", "aç",
]
STOP_KEYWORDS = [
    "durdur", "bitir", "kapat", "iptal",
    "durdursun", "bitirsün", "sonlandır", "sonlandir",
    "dur", "bitti",
]

FUZZY_THRESHOLD = 0.65


# ---------------------------------------------------------------------------
# Yardımcı fonksiyonlar
# ---------------------------------------------------------------------------

def normalize_turkish(text: str) -> str:
    """Türkçe karakterleri ASCII karşılıklarına dönüştürür."""
    replacements = {
        'ş': 's', 'Ş': 'S',
        'ı': 'i', 'İ': 'I',
        'ğ': 'g', 'Ğ': 'G',
        'ü': 'u', 'Ü': 'U',
        'ö': 'o', 'Ö': 'O',
        'ç': 'c', 'Ç': 'C',
    }
    result = text.lower()
    for k, v in replacements.items():
        result = result.replace(k, v)
    return result


def fuzzy_match(word: str, candidates: list[str], threshold: float = FUZZY_THRESHOLD) -> bool:
    """Kelimenin candidate listesindeki herhangi biriyle yeterince benzer olup olmadığını kontrol eder."""
    word_norm = normalize_turkish(word)
    for c in candidates:
        c_norm = normalize_turkish(c)
        # Tam eşleşme veya içerme kontrolü
        if c_norm in word_norm or word_norm in c_norm:
            return True
        # Fuzzy benzerlik
        if SequenceMatcher(None, word_norm, c_norm).ratio() >= threshold:
            return True
    return False


def detect_pomodoro_command(text: str) -> str | None:
    """
    Metin içinde pomodoro komutu arar.
    Dönüş: 'POMODORO_START_25', 'POMODORO_STOP', veya None
    """
    words = re.split(r'[\s,\.!?\'"]+', text.lower())
    words = [w for w in words if w]  # Boş stringleri temizle

    has_pomodoro = False
    has_start = False
    has_stop = False

    for word in words:
        if fuzzy_match(word, POMODORO_KEYWORDS):
            has_pomodoro = True
        if fuzzy_match(word, START_KEYWORDS):
            has_start = True
        if fuzzy_match(word, STOP_KEYWORDS):
            has_stop = True

    if has_pomodoro and has_start:
        return "POMODORO_START_25"
    if has_pomodoro and has_stop:
        return "POMODORO_STOP"

    # Ayrıca tüm cümleyi fuzzy olarak da denemek
    # (Whisper bazen kelimeleri birleştirebiliyor)
    full_norm = normalize_turkish(text)
    for pk in ["pomodoro", "komodoro", "pomidoro"]:
        if pk in full_norm:
            for sk in ["baslat", "basla", "calistir", "ac"]:
                if sk in full_norm:
                    return "POMODORO_START_25"
            for sk in ["durdur", "bitir", "kapat", "iptal", "dur", "bitti"]:
                if sk in full_norm:
                    return "POMODORO_STOP"

    return None


def detect_environment_command(text: str) -> bool:
    """Ortam analizi komutunu algılar."""
    norm = normalize_turkish(text)
    env_triggers = ["ortam", "cevre", "calisma ortam", "oda"]
    action_triggers = ["analiz", "yorumla", "degerlendir", "nasil", "incele", "kontrol"]

    has_env = any(t in norm for t in env_triggers)
    has_action = any(t in norm for t in action_triggers)

    return has_env and has_action


def ask_ollama(prompt: str) -> str:
    payload = {
        "model": OLLAMA_MODEL,
        "prompt": prompt,
        "stream": False
    }
    r = requests.post(OLLAMA_URL, json=payload, timeout=120)
    try:
        r.raise_for_status()
    except requests.exceptions.HTTPError as e:
        print(f"Ollama HTTP Hatasi: {e}")
        print(f"Ollama Detay: {r.text}")
        raise
    data = r.json()
    return data.get("response", "").strip()


def classify_temperature(temp_value: float) -> str:
    if temp_value < 20:
        return "soguk"
    elif temp_value < 26:
        return "ideal"
    elif temp_value < 30:
        return "sicak"
    else:
        return "cok_sicak"


def classify_light(light_value: int) -> str:
    if light_value < 200:
        return "cok_dusuk"
    elif light_value < 500:
        return "dusuk"
    elif light_value < 800:
        return "iyi"
    else:
        return "fazla_parlak"


def classify_noise(noise_value: int) -> str:
    if noise_value < 50:
        return "sessiz"
    elif noise_value < 120:
        return "orta"
    else:
        return "yuksek"


def parse_temp_value(temp_text: str) -> float:
    temp_text = temp_text.lower().replace("c", "").replace("deg", "").strip()
    return float(temp_text)


def parse_int_value(text: str) -> int:
    return int(text.strip())


# ---------------------------------------------------------------------------
# Ana endpoint
# ---------------------------------------------------------------------------

@app.route("/chat", methods=["POST"])
def chat():
    try:
        print("=" * 60)
        print("Istek geldi")

        audio_bytes = request.data
        print("Gelen byte:", len(audio_bytes))

        if not audio_bytes:
            return Response("Ses verisi gelmedi.", status=400, mimetype="text/plain; charset=utf-8")

        pcm = np.frombuffer(audio_bytes, dtype=np.int16)
        if pcm.size == 0:
            return Response("Bos ses verisi.", status=400, mimetype="text/plain; charset=utf-8")

        # Ham sensör verileri header olarak geliyor
        temp_raw = request.headers.get("X-Temp", "28.5C")
        light_raw = request.headers.get("X-Light", "300")
        noise_raw = request.headers.get("X-Noise", "80")

        print(f"Ham sensorler -> Sicaklik: {temp_raw}, Isik: {light_raw}, Gurultu: {noise_raw}")

        temp_value = parse_temp_value(temp_raw)
        light_value = parse_int_value(light_raw)
        noise_value = parse_int_value(noise_raw)

        temp_state = classify_temperature(temp_value)
        light_state = classify_light(light_value)
        noise_state = classify_noise(noise_value)

        print(f"Siniflandirma -> sicaklik: {temp_state}, isik: {light_state}, gurultu: {noise_state}")

        # WAV kaydet
        file_id = str(uuid.uuid4())[:8]
        wav_path = os.path.join(SAVE_DIR, f"input_{file_id}.wav")

        with wave.open(wav_path, "wb") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)
            wf.setframerate(16000)
            wf.writeframes(pcm.tobytes())

        print(f"WAV kaydedildi: {wav_path}")
        print("Transcribe basliyor...")

        segments, info = whisper_model.transcribe(
            wav_path,
            language="tr",
            task="transcribe",
            beam_size=5,
            vad_filter=True
        )

        user_text = " ".join(segment.text.strip() for segment in segments).strip()
        print(f"Kullanici metni: '{user_text}'")

        if not user_text:
            print("Metin bos, tekrar soruluyor (audio).")
            err_wav = generate_tts_wav("Sizi tam anlayamadım, tekrar söyler misiniz?")
            return Response(err_wav, mimetype="audio/wav")

        # ---------------------------------------------------------------
        # 1) Pomodoro komutu: Ollama'ya gitmeden deterministik algıla
        # ---------------------------------------------------------------
        pomodoro_cmd = detect_pomodoro_command(user_text)
        if pomodoro_cmd:
            print(f"Pomodoro komutu algilandi: {pomodoro_cmd}")
            # Pomodoro başlatıldı/durduruldu mesajı hazırla
            print("TTS ses uretiliyor...")
            if "START" in pomodoro_cmd:
                wav_bytes = generate_tts_wav("25 dakikalı Pomodoro başlatıldı, iyi odaklanmalar.")
            else:
                wav_bytes = generate_tts_wav("Pomodoro süresi sonlandırıldı.")
            
            print(f"TTS Bitti. WAV {len(wav_bytes)} byte üretti.")
            resp = Response(wav_bytes, mimetype="audio/wav")
            resp.headers["X-Command"] = pomodoro_cmd
            return resp

        # ---------------------------------------------------------------
        # 2) Ortam analizi komutu
        # ---------------------------------------------------------------
        if detect_environment_command(user_text):
            print("Ortam analizi komutu algilandi")
            prompt = f"""Sen Turkce konusan bir calisma ortami asistanisin.

Kurallar:
- Yalnizca Turkce cevap ver.
- Kisa ve net cevap ver (en fazla 2-3 cumle).
- Baska dil kullanma.
- Sensor verilerini temel alarak ortami yorumla.
- Once kisa degerlendirme yap, sonra kisa oneride bulun.

Sensor verileri:
- Sicaklik: {temp_value}°C ({temp_state})
- Isik seviyesi: {light_value} ({light_state})
- Gurultu seviyesi: {noise_value} ({noise_state})

Kullanicinin istegi: {user_text}""".strip()

        else:
            # ---------------------------------------------------------------
            # 3) Genel soru — Ollama'ya gönder
            # ---------------------------------------------------------------
            prompt = f"""Sen Turkce konusan bir sesli asistansin.

Kurallar:
- Yalnizca Turkce cevap ver.
- Kisa, dogal ve anlasilir cevap ver (en fazla 2-3 cumle).
- Baska dil kullanma.
- Kullaniciyi gereksiz yere duzeltme.
- Metin anlamsizsa sadece: 'Sizi tam anlayamadim, tekrar soyler misiniz?' de.

Kullanici metni: {user_text}""".strip()

        print("Ollama'ya gonderiliyor...")
        answer = ask_ollama(prompt)

        if not answer:
            answer = "Su an cevap uretemedim."

        print(f"Model cevabi: {answer}")
        
        # TTS ile sesi üret
        print("TTS ses uretiliyor...")
        wav_bytes = generate_tts_wav(answer)
        print(f"TTS Bitti. WAV {len(wav_bytes)} byte üretti.")
        
        return Response(wav_bytes, mimetype="audio/wav")

    except Exception as e:
        print(f"HATA: {e}")
        return Response(f"Sunucu hatasi: {e}", status=500, mimetype="text/plain; charset=utf-8")


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)