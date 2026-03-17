# -*- coding: utf-8 -*-
"""
Pomodoro ve ortam komut algılama test scripti.
app.py'deki detect fonksiyonlarını doğrudan import ederek test eder.

Kullanım:
    python test_command_detection.py
"""
import sys
import os

# app.py'yi import edebilmek için path'e ekle
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from app import detect_pomodoro_command, detect_environment_command, normalize_turkish

# ---------------------------------------------------------------------------
# Test verileri
# ---------------------------------------------------------------------------

POMODORO_START_TESTS = [
    # (input_text, beklenen_sonuç)
    ("pomodoro başlat", "POMODORO_START_25"),
    ("pomodoro başla", "POMODORO_START_25"),
    ("Pomodoro başlat", "POMODORO_START_25"),
    ("POMODORO BAŞLAT", "POMODORO_START_25"),
    ("pomodoro'yu başlat", "POMODORO_START_25"),
    ("Pomodoro'yu başlatır mısın", "POMODORO_START_25"),
    ("komodoro baslat", "POMODORO_START_25"),
    ("komodoro başla", "POMODORO_START_25"),
    ("pomidoro basla", "POMODORO_START_25"),
    ("pomodorro başlat", "POMODORO_START_25"),
    ("pomodoro çalıştır", "POMODORO_START_25"),
    ("pomodoro aç", "POMODORO_START_25"),
]

POMODORO_STOP_TESTS = [
    ("pomodoro durdur", "POMODORO_STOP"),
    ("pomodoro bitir", "POMODORO_STOP"),
    ("pomodoro kapat", "POMODORO_STOP"),
    ("pomodoro iptal", "POMODORO_STOP"),
    ("Pomodoro'yu durdur", "POMODORO_STOP"),
    ("komodoro durdur", "POMODORO_STOP"),
    ("pomodoro dur", "POMODORO_STOP"),
]

NON_POMODORO_TESTS = [
    ("bugün hava nasıl", None),
    ("merhaba nasılsın", None),
    ("saat kaç", None),
    ("ortamı analiz et", None),       # Bu env komutu, pomodoro değil
    ("çalışmaya başla", None),         # "başla" var ama "pomodoro" yok
]

ENV_ANALYSIS_TESTS = [
    ("ortamı analiz et", True),
    ("ortami analiz et", True),
    ("çalışma ortamını değerlendir", True),
    ("odayı incele", True),
    ("ortam nasıl", True),
    ("çevreyi kontrol et", True),
    ("merhaba nasılsın", False),
    ("pomodoro başlat", False),
    ("bugün hava nasıl", False),
]

NORMALIZE_TESTS = [
    ("Şekerci", "sekerci"),
    ("IĞDIR", "igdir"),
    ("çöğüş", "cogus"),
    ("ÜÖĞİŞÇ", "uogisç"),  # Büyük İ -> I -> i dönüşümü
]


def run_tests():
    passed = 0
    failed = 0
    total = 0

    print("=" * 60)
    print("POMODORO BAŞLATMA TESTLERİ")
    print("=" * 60)
    for text, expected in POMODORO_START_TESTS:
        total += 1
        result = detect_pomodoro_command(text)
        status = "✓" if result == expected else "✗"
        if result == expected:
            passed += 1
        else:
            failed += 1
        print(f"  {status} '{text}' -> {result} (beklenen: {expected})")

    print()
    print("=" * 60)
    print("POMODORO DURDURMA TESTLERİ")
    print("=" * 60)
    for text, expected in POMODORO_STOP_TESTS:
        total += 1
        result = detect_pomodoro_command(text)
        status = "✓" if result == expected else "✗"
        if result == expected:
            passed += 1
        else:
            failed += 1
        print(f"  {status} '{text}' -> {result} (beklenen: {expected})")

    print()
    print("=" * 60)
    print("POMODORO OLMAYAN METİN TESTLERİ (False Positive Kontrolü)")
    print("=" * 60)
    for text, expected in NON_POMODORO_TESTS:
        total += 1
        result = detect_pomodoro_command(text)
        status = "✓" if result == expected else "✗"
        if result == expected:
            passed += 1
        else:
            failed += 1
        print(f"  {status} '{text}' -> {result} (beklenen: {expected})")

    print()
    print("=" * 60)
    print("ORTAM ANALİZİ TESTLERİ")
    print("=" * 60)
    for text, expected in ENV_ANALYSIS_TESTS:
        total += 1
        result = detect_environment_command(text)
        status = "✓" if result == expected else "✗"
        if result == expected:
            passed += 1
        else:
            failed += 1
        print(f"  {status} '{text}' -> {result} (beklenen: {expected})")

    print()
    print("=" * 60)
    print(f"SONUÇ: {passed}/{total} test başarılı, {failed} başarısız")
    print("=" * 60)

    return failed == 0


if __name__ == "__main__":
    success = run_tests()
    sys.exit(0 if success else 1)
