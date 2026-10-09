from pathlib import Path
import math
import random
import struct
import wave

out = Path("benchmark_files")
out.mkdir(exist_ok=True)

# 1. 高重複性文字
text = "Hello TextLink! 這是 Huffman 壓縮測試。1234567890\n"
(out / "text_repeat.txt").write_text(
    text * 30000, encoding="utf-8"
)

# 2. 混合文字
rng = random.Random(2026)
symbols = (
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789"
    "你好世界測試資料傳輸壓縮編碼"
)

with (out / "text_mixed.txt").open(
    "w", encoding="utf-8"
) as f:
    for _ in range(40000):
        line = "".join(rng.choices(symbols, k=40))
        f.write(line + "\n")

# 3. 規律正弦波 WAV
def make_wav(path, noise=False):
    rate = 8000
    count = rate * 70
    rng = random.Random(2026)

    with wave.open(str(path), "wb") as wav:
        wav.setnchannels(1)
        wav.setsampwidth(2)
        wav.setframerate(rate)

        for i in range(count):
            if noise:
                sample = rng.randint(-32768, 32767)
            else:
                sample = int(
                    12000 * math.sin(
                        2 * math.pi * 440 * i / rate
                    )
                )

            wav.writeframesraw(
                struct.pack("<h", sample)
            )

make_wav(out / "audio_sine.wav")
make_wav(out / "audio_noise.wav", noise=True)

print("測試檔案建立完成：")
for path in out.iterdir():
    print(path.name, path.stat().st_size, "bytes")