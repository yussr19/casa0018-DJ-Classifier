# DJ Sound Classifier — CASA0018

**Author:** yussr19  
**GitHub:** [casa0018-DJ-Classifier](https://github.com/yussr19/casa0018-DJ-Classifier)  
**Edge Impulse Project:** https://studio.edgeimpulse.com/public/965867/live

---

## What It Does

The DJ Sound Classifier is a real-time embedded audio classification system that listens for acoustic events in a DJ set and triggers reactive LED lighting responses entirely on-device — no cloud, no internet connection required.

The system classifies three audio classes:

| Class | LED Response |
|-------|-------------|
| Bass drop | 🔵 Blue chasing effect |
| Kick drop | 🔴 Solid red |
| Control noise | 🟢 Solid green |

![DJ Classifier enclosure showing green LED response](IMG_7355.jpg)
*The custom 3D printed enclosure showing a control noise (green) classification*

---

## Motivation

The UK's grassroots music venue scene has faced a prolonged crisis — over 125 venues closed in 2023 alone, driven by rising costs including expensive lighting and production equipment. This project prototypes a low-cost embedded alternative to professional audio-reactive lighting systems, exploring whether machine learning at the edge could assist resource-constrained venues.

---

## Hardware

| Component | Details | Approximate Cost |
|-----------|---------|-----------------|
| Arduino Nano 33 BLE Sense | Microcontroller with onboard PDM mic | £35 |
| WS2812B 16-LED NeoPixel strip | Addressable RGB LED strip | £5 |
| 3×AA battery pack | 4.5V power supply | £2 |
| 330Ω resistor | Data line protection | <£1 |
| 100µF capacitor | Power rail smoothing | <£1 |
| Breadboard + jumper wires | Circuit assembly | £3 |
| 3D printed enclosure | YUSSR_DOME.stl + light_boxx.stl | £2 (filament) |

**Estimated total cost: ~£48**

---

## Reproducing This Project

Follow these steps exactly to replicate the system from scratch.

### Step 1 — Print the Enclosure

Print both STL files from the `/enclosure` folder:
- `light_boxx.stl` — the base box housing the breadboard and Arduino
- `DOME.stl` — the dome that diffuses the LED light

Recommended print settings: PLA, 0.2mm layer height, 20% infill. The dome benefits from printing in translucent or white filament for best light diffusion.

---

### Step 2 — Assemble the Circuit

Wire the components as follows:

```
3×AA Battery (+) ──────────────────────── 5V power rail
3×AA Battery (-) ──────────────────────── GND power rail

Arduino 3.3V ──── 100µF capacitor (+) ─── 5V rail
                  100µF capacitor (-) ─── GND rail

Arduino D6 ─────── 330Ω resistor ─────── WS2812B DIN
Arduino GND ───────────────────────────── WS2812B GND
5V rail ────────────────────────────────── WS2812B 5V
```

**Important:** Always connect the 330Ω resistor between D6 and the LED strip data line — this protects the first LED from signal spikes. The 100µF capacitor across the power rails prevents voltage dips when LEDs switch state, which can cause the Arduino to reset mid-inference.

Place the Arduino and breadboard inside `light_boxx.stl`. Thread the LED strip around the inside of `YUSSR_DOME.stl` and place the dome over the base.

---

### Step 3 — Set Up Edge Impulse

You can either use the existing trained model or retrain from scratch.

#### Option A — Use the existing model (recommended)

1. Go to https://studio.edgeimpulse.com/public/965867/live
2. Click **Clone this project** to copy it to your account
3. Go to **Deployment** → **Arduino library** → **Build** → Download ZIP
4. Skip to Step 4

#### Option B — Retrain from scratch

1. Create a new Edge Impulse project at https://studio.edgeimpulse.com
2. Connect your Arduino Nano 33 BLE Sense via the Edge Impulse daemon:

```bash
npm install -g edge-impulse-cli
edge-impulse-daemon --clean
```

3. Collect data — record 2-second samples at 16kHz for each class:
   - **bass_drop** — record during sustained bass-heavy sections of DJ sets
   - **kick_drop** — record during rhythmic kick drum patterns
   - **control_noise** — record ambient crowd and room noise between events
   - Target: ~115 samples per class, mix of home and venue recordings

4. Create the impulse:
   - Time series input: 2000ms window, 500ms stride, 16000Hz
   - Processing block: **Audio (MFE)**
   - Learning block: **Classification**

5. Configure MFE parameters:
   - Frame length: 0.02
   - Frame stride: 0.01
   - Filter number: **6**
   - FFT length: **256**
   - Low frequency: 0
   - High frequency: **300**

6. Generate features → train classifier:
   - Training cycles: **100**
   - Learning rate: 0.005
   - Data augmentation: **off**
   - In Keras expert mode, update the reshape layer to:

```python
Reshape((int(input_length / 6), 6))
```

7. Deploy as Arduino library

---

### Step 4 — Install Arduino Libraries

1. Open Arduino IDE
2. Install the Adafruit NeoPixel library:
   - Tools → Manage Libraries → search **Adafruit NeoPixel** → Install
3. Install the Edge Impulse library:
   - Sketch → Include Library → Add .ZIP Library → select the downloaded ZIP from Step 3

---

### Step 5 — Upload the Sketch

1. Open `DJ_LED2/DJ_LED2.ino` from this repository
2. Select board: **Arduino Nano 33 BLE Sense**
3. Select the correct port
4. Click **Upload**

If the Arduino IDE upload fails, use bossac via terminal instead:

```bash
# Find your port
ls /dev/cu.usbmodem*

# Upload via bossac (replace PORT with your port number)
bossac -p /dev/cu.usbmodem[PORT] -e -w -v -R DJ_LED2.ino.bin
```

---

### Step 6 — Power and Test

1. Disconnect USB and connect the 3×AA battery pack
2. The LED strip will flash **white** on startup confirming the system is live
3. Play music through a speaker near the device:
   - Bass-heavy music → LEDs should show **blue chasing** effect
   - Kick-heavy patterns → LEDs should show **solid red**
   - Silence or ambient noise → LEDs should show **solid green**

> **Note:** The system works best positioned 30–50cm from the speaker. Performance degrades in loud venue environments due to the domain gap identified in testing — direct line-in connection from a DJ mixer would substantially improve reliability.

---

## ML Model Details

- **Platform:** Edge Impulse (Project ID: 965867)
- **Preprocessing:** MFE, 300Hz low-pass, 6 filters, FFT 256
- **Architecture:** 1D CNN — Input → Reshape → Conv1D (8 filters) → Dropout 0.25 → Conv1D (16 filters) → Dropout 0.25 → Flatten → Softmax (3 classes)
- **Training:** 100 cycles, learning rate 0.005, batch size 32, no augmentation
- **Deployment:** INT8 quantised via EON compiler
- **Inference:** 5ms per cycle, 13.8KB RAM, 46.5KB flash

---

## Dataset

| Class | Samples | Source |
|-------|---------|--------|
| bass_drop | 116 | 195 home + 150 live venue recordings |
| kick_drop | 115 | Collected Apr 25 2026 at live venue |
| control_noise | 115 | 2s WAV, 16kHz mono |
| **Total** | **346** | 80/20 train/test split |

---

## Results Summary

| Experiment | Configuration | Accuracy | Weighted F1 |
|-----------|---------------|----------|-------------|
| Broadband Spectrogram | No filter | 30.0% | 0.14 |
| MFE 200Hz | Low-pass 200Hz | 52.9% | 0.51 |
| **MFE 300Hz** | **Low-pass 300Hz — best** | **57.1%** | **0.58** |
| MFE 500Hz | Low-pass 500Hz | 57.1% | 0.57 |
| MFCC Broadband | No filter | 52.9% | 0.53 |
| MFCC 300Hz | Low-pass 300Hz | 44.3% | 0.42 |
| Float32 vs INT8 | Quantisation comparison | 54.3% → 57.1% | — |
| Venue Test 1 | Live venue (80/20 split) | 21.74% | 0.37 |
| Venue Test 2 | Home train / venue test | 35.71% | 0.46 |

---

## Known Limitations

- Significant domain gap between home and live venue recordings (35+ percentage point drop in accuracy)
- Kick drop classification is weak (F1: 0.40) due to spectral overlap with bass drops in the sub-300Hz range
- Microphone-based approach captures environmental acoustics — direct mixer line-in connection would substantially improve venue robustness

---




Academic Year 2025/26
