import numpy as np
from scipy.io import wavfile
import matplotlib.pyplot as plt

import soundfile as sf

# WAV-Datei einlesen
data, sample_rate = sf.read('15-08-28_435500000Hz.wav')

data = data[:,0]

# # Zeitachse erstellen
# time = np.arange(0, len(data)) / sample_rate

# # Plot Zeitverlauf
# plt.figure(figsize=(15, 6))
# plt.subplot(2, 1, 1)
# plt.plot(time, data)
# plt.xlabel('Zeit [s]')
# plt.ylabel('Amplitude')
# plt.title('Zeitverlauf')

# # Fourier-Transformation für das Spektrum
# freqs = np.fft.fftfreq(len(data), 1/sample_rate)
# fft = np.fft.fft(data)

# # Plot Spektrum
# plt.subplot(2, 1, 2)
# plt.plot(np.abs(freqs), np.abs(fft))
# plt.xlabel('Frequenz [Hz]')
# plt.ylabel('Magnitude')
# plt.title('Spektrum')
# plt.tight_layout()
# plt.show()


from scipy.signal import stft
from mpl_toolkits.mplot3d import Axes3D

# Kurze Fourier-Transformation (STFT)
frequencies, times, Zxx = stft(data, fs=sample_rate, nperseg=4096, noverlap=2048)

# Betrag der STFT nehmen, da sie komplex ist
Zxx = np.abs(Zxx)

# Wasserfalldiagramm erstellen
fig = plt.figure(figsize=(12, 8))
ax = fig.add_subplot(111, projection='3d')

# Jede Frequenzlinie als separate Linie im 3D-Plot zeichnen
for i in range(Zxx.shape[0]):
    # Nur jede 10. Frequenzlinie zeichnen, um den Plot übersichtlicher zu machen
    if i % 10 == 0:
        ax.plot(times, frequencies[i]*np.ones(Zxx.shape[1]), 10*np.log10(Zxx[i, :]))

ax.set_xlabel('Zeit [s]')
ax.set_ylabel('Frequenz [Hz]')
ax.set_zlabel('Magnitude [dB]')
ax.set_title('Wasserfalldiagramm')
plt.show()
