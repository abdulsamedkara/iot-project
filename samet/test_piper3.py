import wave, io
from piper import PiperVoice

voice = PiperVoice.load("./piper_models/tr_TR-dfki-medium.onnx")

def test_tts(text):
    if not text.endswith(('.', '!', '?')):
        text += '.'
    buf = io.BytesIO()
    with wave.open(buf, 'wb') as wf:
        voice.synthesize_wav(text, wf)
    return buf.getvalue()

start = test_tts("25 dakikalı Pomodoro başlatıldı, iyi odaklanmalar.")
stop = test_tts("Pomodoro süresi sonlandırıldı.")

print(f"START bytes: {len(start)}")
print(f"STOP bytes: {len(stop)}")
