import numpy as np
import argparse
import soundfile as sf
from PIL import Image
import json
import os

def load_image_to_bin(img_path: str, out_path: str):
    img = Image.open(img_path).convert('L')
    arr = np.array(img, dtype=np.float32)
    img_min, img_max = arr.min(), arr.max()
    norm = (arr - img_min) / (img_max - img_min + 1e-9)
    norm.tofile(out_path)
    return norm.shape, img_min, img_max

def load_wav_to_bin(wav_path: str, out_path: str):
    data, sr = sf.read(wav_path)
    if len(data.shape) > 1:
        data = data[:, 0]  # Take first channel
    data = data.astype(np.float32)
    data.tofile(out_path)
    return len(data), sr

def main():
    parser = argparse.ArgumentParser(description="Prepare inputs for DSP")
    parser.add_argument('--image', required=True)
    parser.add_argument('--wav', required=True)
    parser.add_argument('--out_img_bin', required=True)
    parser.add_argument('--out_wav_bin', required=True)
    parser.add_argument('--params_json', required=True)
    
    args = parser.parse_args()
    
    os.makedirs(os.path.dirname(args.out_img_bin) or '.', exist_ok=True)
    
    shape, vmin, vmax = load_image_to_bin(args.image, args.out_img_bin)
    alen, sr = load_wav_to_bin(args.wav, args.out_wav_bin)
    
    params = {
        'image_height': shape[0],
        'image_width': shape[1],
        'image_min': float(vmin),
        'image_max': float(vmax),
        'audio_length': alen,
        'audio_sr': sr
    }
    
    with open(args.params_json, 'w') as f:
        json.dump(params, f, indent=4)
        
    print(f"Inputs prepared and saved. Params: {params}")

if __name__ == '__main__':
    main()
