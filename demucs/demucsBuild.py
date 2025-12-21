from demucs import pretrained
from demucs.apply import apply_model
import os
import torch
import soundfile as sf
import torchaudio
import sys

# Select device (GPU if available)
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(f"Using device: {device}")

# Load pre-trained model
model = pretrained.get_model("htdemucs")  # can also use 'demucs' or 'htdemucs_ft'
model = model.to(device)
model.eval()

filenames = sys.argv[1:]

for name in filenames:

    audio_path = name
    data, sr = sf.read(audio_path)  # waveform shape: (channels, samples)

    # Demucs expects (batch, channels, samples)
    #waveform = torch.tensor(data.T, dtype=torch.float32).unsqueeze(0).to(device)  # shape -> (1, channels, samples)

    if data.ndim == 1:  # mono
        waveform = torch.tensor(data, dtype=torch.float32).unsqueeze(0).unsqueeze(0).to(device)
    else:  # stereo or more channels
        waveform = torch.tensor(data.T, dtype=torch.float32).unsqueeze(0).to(device)

    if waveform.shape[1] == 1:
        waveform = waveform.repeat(1, 2, 1)  # (batch, channels*2, samples)



    with torch.no_grad():
        # apply_model handles chunking, LSTM, overlap-add internally
        sources = apply_model(model, waveform, shifts=1, split=True, progress=True)
        # sources shape: (batch, n_sources, channels, samples)

    output_dir = "separated_stems"
    os.makedirs(output_dir, exist_ok=True)

    sources = sources.squeeze(0).cpu()  # remove batch dimension

    for i, name in enumerate(model.sources):
        stem = sources[i].cpu()  # shape: (channels, samples)
        # Convert to numpy array with shape (samples, channels)
        stem_np = stem.numpy().T
        sf.write(os.path.join(output_dir, f"{name}.wav"), stem_np, sr)
