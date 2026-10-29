import sounddevice as sd
import numpy as np
import time

devices = sd.query_devices()
for i, d in enumerate(devices):
    if d["max_input_channels"] > 0:
        print(i, d["name"])

dev = int(input("A Pico mikrofon eszközszáma: "))

data = sd.rec(
    int(10 * 48000),
    samplerate=48000,
    channels=2,
    dtype="int16",
    device=dev
)

start = time.perf_counter()
sd.wait()
elapsed = time.perf_counter() - start

print(f"Idő: {elapsed:.3f} s")
print(f"Mintapárok: {len(data)}")
print(f"Csatornaminták: {data.size}")
print(f"Átlagos mintasebesség: {len(data) / elapsed:.2f} pár/s")