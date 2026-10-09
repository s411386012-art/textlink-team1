
import wave
import csv
import numpy as np
from pathlib import Path

# WAV 檔案位置
wav_path = Path("benchmark_real/audio_music_20s.wav")

# 輸出 CSV
output_path = Path("benchmarks/figures/audio_music_histogram.csv")
output_path.parent.mkdir(parents=True, exist_ok=True)

# 讀取 WAV
with wave.open(str(wav_path), "rb") as wav:
    channels = wav.getnchannels()
    sample_width = wav.getsampwidth()
    sample_rate = wav.getframerate()
    frames = wav.getnframes()
    raw_data = wav.readframes(frames)

if sample_width != 2:
    raise ValueError("此程式僅支援 16-bit PCM WAV")

# 16-bit PCM 轉成整數取樣值
samples = np.frombuffer(raw_data, dtype="<i2")

# 將 -32768 ~ 32767 分成 64 個區間
counts, edges = np.histogram(
    samples,
    bins=64,
    range=(-32768, 32768)
)

# 輸出 Excel 可以使用的 CSV
with output_path.open("w", newline="", encoding="utf-8-sig") as f:
    writer = csv.writer(f)
    writer.writerow(["sample_range", "count"])

    for i, count in enumerate(counts):
        left = int(edges[i])
        right = int(edges[i + 1]) - 1
        writer.writerow([f"{left} ~ {right}", int(count)])

print("WAV 檔案：", wav_path)
print("聲道數：", channels)
print("取樣率：", sample_rate, "Hz")
print("總 Frames：", frames)
print("總 Samples：", len(samples))
print("CSV 已產生：", output_path)
