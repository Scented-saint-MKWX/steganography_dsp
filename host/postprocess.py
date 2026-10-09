import numpy as np
import argparse
import soundfile as sf
from PIL import Image
import os
import json
from ref_pipeline import compute_psnr, compute_ssim, compute_mse, compute_snr

def bin_to_image(bin_path: str, width: int, height: int, vmin: float, vmax: float, out_path: str):
    data = np.fromfile(bin_path, dtype=np.float32)
    data = data.reshape((height, width))
    data = np.clip(data, 0.0, 1.0)
    denorm = data * (vmax - vmin) + vmin
    img = Image.fromarray(denorm.astype(np.uint8), mode='L')
    img.save(out_path)
    return data

def bin_to_wav(bin_path: str, sample_rate: int, out_path: str):
    data = np.fromfile(bin_path, dtype=np.float32)
    sf.write(out_path, data, sample_rate)
    return data

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--extracted_img_bin', required=True)
    parser.add_argument('--stego_wav_bin', required=True)
    parser.add_argument('--orig_img_bin', required=True)
    parser.add_argument('--orig_wav_bin', required=True)
    parser.add_argument('--params_json', required=True)
    parser.add_argument('--out_img', required=True)
    parser.add_argument('--out_wav', required=True)
    
    args = parser.parse_args()
    
    with open(args.params_json, 'r') as f:
        params = json.load(f)
        
    w, h = params['image_width'], params['image_height']
    vmin, vmax = params['image_min'], params['image_max']
    sr = params['audio_sr']
    
    ext_img_data = bin_to_image(args.extracted_img_bin, w, h, vmin, vmax, args.out_img)
    stego_wav_data = bin_to_wav(args.stego_wav_bin, sr, args.out_wav)
    
    orig_img_data = np.fromfile(args.orig_img_bin, dtype=np.float32).reshape((h, w))
    orig_wav_data = np.fromfile(args.orig_wav_bin, dtype=np.float32)
    
    psnr = compute_psnr(orig_img_data, ext_img_data)
    ssim_val = compute_ssim(orig_img_data, ext_img_data)
    mse = compute_mse(orig_img_data, ext_img_data)
    snr = compute_snr(orig_wav_data, stego_wav_data[:len(orig_wav_data)])
    
    print("--- Metrics ---")
    print(f"Image PSNR: {psnr:.2f} dB")
    print(f"Image SSIM: {ssim_val:.4f}")
    print(f"Image MSE:  {mse:.6f}")
    print(f"Audio SNR:  {snr:.2f} dB")
    
if __name__ == '__main__':
    main()
