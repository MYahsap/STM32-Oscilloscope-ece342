# 📊 STM32 Dual-Screen Oscilloscope (ECE342)

A **dual-screen digital oscilloscope** built on the STM32F446ZE microcontroller. This project demonstrates real-time signal acquisition, visualization, and measurement using embedded systems concepts like ADC, DMA, interrupts, and I2C communication.

---

## 🚀 Features

- 📈 **Real-time waveform display**
- 📟 **Dual OLED screens**
  - Screen 1: Signal waveform
  - Screen 2: Settings & measurements
- 🎛️ **Interactive controls via hardware buttons**
  - Horizontal scaling (µs/div)
  - Vertical scaling (mV/div)
  - Trigger level adjustment
  - Reset functionality
- ⚡ **Efficient signal acquisition**
  - Timer-triggered ADC
  - DMA for continuous, non-blocking sampling
- 📊 **Signal measurements**
  - Frequency
  - Peak-to-peak voltage (Vpp)
  - Max/Min voltage

---

## 🧠 System Architecture

The system uses:
- **ADC + DMA** → Continuous sampling into a buffer  
- **Timer** → Controls sampling rate  
- **Display Buffer** → Stores waveform data  
- **Interrupt-driven UI** → Handles button input  
- **I2C OLED Displays** → Output visualization  

---

## ⚙️ Hardware Components

- STM32F446ZE Microcontroller  
- 2 × OLED Displays (I2C, 128×64)  
- Push Buttons (GPIO interrupts via EXTI)  
- Signal Conditioning Circuit (input range scaling)  
- LEDs (debugging indicators)  

---

## 📉 Performance

| Component | Theoretical | Actual |
|----------|------------|--------|
| ADC Sampling Rate | 2.4 MSPS | ~160 kHz |
| OLED FPS (I2C @100kHz) | ~40 FPS | ~10 FPS |

### ⚠️ Bottleneck
- I2C communication limits display refresh rate significantly.
- Sending full frame buffers to two screens introduces visible delay.

---

## 🛠️ Key Implementation Details

- **Rolling buffer (128 samples)** for waveform display  
- **Interrupt-based UI** for responsive control  
- **Custom waveform rendering logic**  
- **Optimized calculations** for real-time performance  

---

## 📂 Code Structure

- ADC + DMA: Continuous sampling system  
- OLED Drive: Modified for dual-screen support  
- Waveform Renderer: Displays sampled data  
- Measurements: Frequency & voltage calculations  
- Interrupt Handlers: Button controls  

---

## 🧪 Limitations

- Limited by **I2C bandwidth**
- No FFT (Fast Fourier Transform) due to time constraints
- No high-voltage protection ⚠️

---

## 🔮 Future Improvements

- 🔁 Switch from **I2C → SPI (with DMA)** for faster display
- 📊 Add **FFT for frequency spectrum analysis**
- 🔊 Audio signal processing integration
- ⚡ Improve sampling resolution and buffer size

---

## ⚠️ Safety Note

This oscilloscope is designed for **low-voltage signals (0–3.3V)**.  
Connecting high-voltage inputs may damage the system and pose safety risks.

---

## ⭐ Demo

_https://drive.google.com/drive/folders/14siSGnDFwhnzH1Lk1-8TJdkh_tfUSIDt_
