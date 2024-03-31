import numpy as np
import matplotlib.pyplot as plt
import soundfile as sf
from matplotlib.widgets import Slider, Button

# Funktion zur Berechnung des Spektrums
def calculate_spectrum(data, sample_rate, window_size=1024, overlap=0.5):
    hop_size = int(window_size * (1 - overlap))
    num_samples = len(data)
    num_frames = int(np.ceil((num_samples - window_size) / hop_size)) + 1
    spectrum = np.zeros((num_frames, window_size // 2 + 1))

    for i in range(num_frames):
        start = i * hop_size
        end = min(start + window_size, num_samples)
        frame = data[start:end] * np.hanning(end - start)
        spectrum[i] = np.abs(np.fft.rfft(frame, window_size))

    freqs = np.fft.rfftfreq(window_size, 1 / sample_rate)
    times = np.arange(num_frames) * hop_size / sample_rate
    return times, freqs, spectrum

# Einlesen der WAV-Datei
filename = '15-08-28_435500000Hz.wav'
data, sample_rate = sf.read(filename)
data = np.mean(data,axis=1)


# Berechnung des Spektrums
times, freqs, spectrum = calculate_spectrum(data, sample_rate)

# xmin,xmax = 0.190, 0.385
# xmin,xmax = 0, 0.6421504
xmin,xmax = 0.19, 0.385
ymin,ymax = 2.4e6, 2.6e6

# Find the indices corresponding to the desired frequency range
freq_min_index = np.argmax(freqs >= ymin)
freq_max_index = np.argmax(freqs >= ymax)

# Find the indices corresponding to the desired time range
time_min_index = np.argmax(times >= xmin)
time_max_index = np.argmax(times >= xmax)

# Slice the spectrum array to include only the desired frequency range
spectrum = spectrum[:, freq_min_index:freq_max_index]
# times = times[time_min_index:time_max_index]

# print(times.max())

# Erstellen des Plots
fig, ax = plt.subplots(figsize=(10, 6))
img = ax.imshow(spectrum.T, aspect='auto', origin='lower', extent=[times.min(), times.max(), freqs.min(), freqs.max()])
ax.set_xlabel('Time (s)')
ax.set_ylabel('Frequency (Hz)')
ax.set_title('Spectrogram')
# Set the y-axis limits to show only the desired frequency range
ax.set_ylim(freqs.min(), freqs.max())
ax.set_xlim(xmin,xmax)
fig.colorbar(img, ax=ax, label='Magnitue')

# Hinzufügen von Widgets für Interaktionen
axzoom = plt.axes([0.1, 0.05, 0.65, 0.03])
axpan = plt.axes([0.1, 0.01, 0.65, 0.03])
szoom = Slider(axzoom, 'Zoom', 1, 20.0, valinit=1)
span = Slider(axpan, 'Pan', xmin, xmax, valinit=xmin)

def update(val):
    zoom = szoom.val
    pan = span.val
    ax.set_xlim(pan, pan + (xmax - xmin) / zoom)
    fig.canvas.draw_idle()

szoom.on_changed(update)
span.on_changed(update)

plt.show()
