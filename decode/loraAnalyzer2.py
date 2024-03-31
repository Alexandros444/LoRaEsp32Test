import numpy as np
from scipy.io import wavfile
import matplotlib.pyplot as plt

import soundfile as sf

# WAV-Datei einlesen
data, sample_rate = sf.read('15-08-28_435500000Hz.wav')

data0 = np.mean(data,axis=1)
# data0 = data[:,0]
data1 = data[:,1]



# Zeitachse erstellen
time = np.arange(0, len(data0)) / sample_rate

# Plot Zeitverlauf
plt.figure(figsize=(15, 6))
plt.subplot(2, 1, 1)
plt.plot(time, data0)
plt.xlabel('Zeit [s]')
plt.ylabel('Amplitude')
plt.title('Zeitverlauf')

# Fourier-Transformation für das Spektrum
freqs = np.fft.fftfreq(len(data0), 1/sample_rate)
fft = np.fft.fft(data0)

# Plot Spektrum
plt.subplot(2, 1, 2)
plt.plot(np.abs(freqs), np.abs(fft))
plt.xlabel('Frequenz [Hz]')
plt.ylabel('Magnitude')
plt.title('Spektrum')
plt.tight_layout()
plt.show()


# # Zeitachse erstellen
# time = np.arange(0, len(data1)) / sample_rate

# # Plot Zeitverlauf
# plt.figure(figsize=(15, 6))
# plt.subplot(2, 1, 1)
# plt.plot(time, data1)
# plt.xlabel('Zeit [s]')
# plt.ylabel('Amplitude')
# plt.title('Zeitverlauf')

# # Fourier-Transformation für das Spektrum
# freqs = np.fft.fftfreq(len(data1), 1/sample_rate)
# fft = np.fft.fft(data1)

# # Plot Spektrum
# plt.subplot(2, 1, 2)
# plt.plot(np.abs(freqs), np.abs(fft))
# plt.xlabel('Frequenz [Hz]')
# plt.ylabel('Magnitude')
# plt.title('Spektrum')
# plt.tight_layout()
# plt.show()