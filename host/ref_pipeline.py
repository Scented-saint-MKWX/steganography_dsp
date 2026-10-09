"""
ref_pipeline.py - Golden reference model for Image-in-Audio Steganography.

Implements the full embed and extract pipeline in NumPy/SciPy/PyWavelets.
This is the ground truth that the C implementation must match.

See EXPLANATION.md for the algorithm specification.
"""
import numpy as np
import pywt
import scipy.fft
import scipy.signal
import json
import os
import sys
from pathlib import Path
from skimage.metrics import structural_similarity as ssim
from PIL import Image

from prng import fisher_yates_permutation, inverse_permutation, key_check_hash


# ═══════════════════════════════════════════════════════════════════════════
# Image Loading & Normalization
# ═══════════════════════════════════════════════════════════════════════════

def load_image(path: str) -> tuple[np.ndarray, float, float]:
    """Load grayscale image, normalize to [0,1], return (img_float, min, max)."""
    img = Image.open(path).convert('L')
    img_arr = np.array(img, dtype=np.float32)
    img_min = float(img_arr.min())
    img_max = float(img_arr.max())
    if img_max - img_min < 1e-9:
        img_float = np.zeros_like(img_arr)
    else:
        img_float = (img_arr - img_min) / (img_max - img_min)
    return img_float, img_min, img_max


def denormalize_image(img_float: np.ndarray, img_min: float, img_max: float) -> np.ndarray:
    """Reverse normalization: map [0,1] back to [img_min, img_max]."""
    return img_float * (img_max - img_min) + img_min


# ═══════════════════════════════════════════════════════════════════════════
# Hermitian Symmetric Spectrum Construction ("Make Pair")
# ═══════════════════════════════════════════════════════════════════════════

def make_pair(img: np.ndarray, N: int = None) -> np.ndarray:
    """
    Construct Hermitian-symmetric spectra from image columns.

    For an H-row image with N-point FFT (N = 2*H):
      spectrum[0]     = img[0, j]       (DC bin, real)
      spectrum[k]     = img[k, j]       for k = 1..H-1 (positive freqs, real)
      spectrum[H]     = 0               (Nyquist bin, real)
      spectrum[N-k]   = conj(img[k, j]) for k = 1..H-1 (negative freqs = mirror)

    Since image values are real, conjugate = same value, so all imaginary parts
    are zero. The IFFT of this Hermitian spectrum produces a real signal.

    See EXPLANATION.md Section 3.1.
    """
    H, W = img.shape
    if N is None:
        N = 2 * H
    assert N == 2 * H, f"FFT size N={N} must be 2*H={2*H}"

    spectrum = np.zeros((N, W), dtype=np.complex64)
    for j in range(W):
        # DC bin (index 0): real
        spectrum[0, j] = img[0, j]
        # Positive frequency bins 1..H-1
        for k in range(1, H):
            spectrum[k, j] = img[k, j]
        # Nyquist bin (index H): set to 0 (real)
        spectrum[H, j] = 0.0
        # Negative frequency bins: conjugate mirror
        # spectrum[N-k] = conj(spectrum[k]) = img[k, j] since img is real
        for k in range(1, H):
            spectrum[N - k, j] = img[k, j]  # conj of real = same

    return spectrum


def unmake_pair(spectrum: np.ndarray, height: int, width: int) -> np.ndarray:
    """
    Extract H pixel values from each column of the spectrum.
    Inverse of make_pair: take the real part of bins 0..H-1.

    See EXPLANATION.md Section 3.1.
    """
    img = np.zeros((height, width), dtype=np.float32)
    for j in range(width):
        for k in range(height):
            img[k, j] = np.real(spectrum[k, j])
    return img


# ═══════════════════════════════════════════════════════════════════════════
# ISTFT / STFT (No Overlap)
# ═══════════════════════════════════════════════════════════════════════════

def istft_no_overlap(spectrum: np.ndarray, N: int) -> np.ndarray:
    """
    Inverse STFT with no overlap. Each column -> one N-point IFFT frame.

    The spectrum has Hermitian symmetry, so IFFT output should be real.
    We keep only the real part and assert imaginary is negligible.

    See EXPLANATION.md Section 3.2.
    """
    _, W = spectrum.shape
    frames = []
    for j in range(W):
        col = spectrum[:, j]
        frame = scipy.fft.ifft(col, n=N)
        # Verify imaginary part is negligible
        max_imag = np.max(np.abs(np.imag(frame)))
        assert max_imag < 1e-4, f"Column {j}: max imag = {max_imag} (should be ~0)"
        frames.append(np.real(frame).astype(np.float32))
    return np.concatenate(frames)


def apply_kaiser_window_inverse(signal: np.ndarray, N: int,
                                 beta: float, epsilon: float = 1e-7) -> np.ndarray:
    """
    Divide each N-sample frame by the Kaiser window.

    At overlap=0, the analysis window must be undone by dividing.
    Guard against division by near-zero with epsilon clamping.

    WARNING: Large beta (>= ~15) makes window values very small at edges,
    which amplifies noise when dividing. This destroys the image (paper Table 1).

    See EXPLANATION.md Section 3.3.
    """
    window = scipy.signal.windows.kaiser(N, beta).astype(np.float32)
    # Clamp near-zero values
    window_safe = np.where(np.abs(window) < epsilon, epsilon, window)
    window_inv = 1.0 / window_safe

    num_frames = len(signal) // N
    out = np.zeros_like(signal)
    for i in range(num_frames):
        s = i * N
        e = s + N
        out[s:e] = signal[s:e] * window_inv
    return out


def apply_kaiser_window_forward(signal: np.ndarray, N: int,
                                 beta: float) -> np.ndarray:
    """
    Multiply each N-sample frame by the Kaiser window.
    Used in the extract path before FFT (analysis windowing).
    """
    window = scipy.signal.windows.kaiser(N, beta).astype(np.float32)
    num_frames = len(signal) // N
    out = np.zeros_like(signal)
    for i in range(num_frames):
        s = i * N
        e = s + N
        out[s:e] = signal[s:e] * window
    return out


def stft_reconstruct(signal: np.ndarray, N: int, beta: float) -> np.ndarray:
    """
    STFT: apply Kaiser window to each frame, then FFT.
    Returns complex spectrum of shape (N, num_frames).
    """
    window = scipy.signal.windows.kaiser(N, beta).astype(np.float32)
    num_frames = len(signal) // N
    spectrum = np.zeros((N, num_frames), dtype=np.complex64)
    for i in range(num_frames):
        s = i * N
        e = s + N
        frame = signal[s:e] * window
        spectrum[:, i] = scipy.fft.fft(frame, n=N)
    return spectrum


# ═══════════════════════════════════════════════════════════════════════════
# Image <-> Signal Conversion
# ═══════════════════════════════════════════════════════════════════════════

def image_to_signal(img: np.ndarray, N: int = None,
                     beta: float = 1.0, epsilon: float = 1e-7) -> np.ndarray:
    """
    Convert normalized grayscale image to target signal.
    Steps: make_pair -> ISTFT -> divide by Kaiser window.

    See EXPLANATION.md Section 3.
    """
    H, W = img.shape
    if N is None:
        N = 2 * H
    spectrum = make_pair(img, N)
    signal = istft_no_overlap(spectrum, N)
    signal = apply_kaiser_window_inverse(signal, N, beta, epsilon)
    return signal


def signal_to_image(signal: np.ndarray, H: int, W: int,
                     N: int = None, beta: float = 1.0) -> np.ndarray:
    """
    Convert target signal back to grayscale image.
    Steps: apply Kaiser window -> FFT -> unmake_pair.

    See EXPLANATION.md Section 3.
    """
    if N is None:
        N = 2 * H
    spectrum = stft_reconstruct(signal, N, beta)
    img = unmake_pair(spectrum, H, W)
    return img


# ═══════════════════════════════════════════════════════════════════════════
# Encryption (Keyed Permutation)
# ═══════════════════════════════════════════════════════════════════════════

def encrypt_signal(signal: np.ndarray, key: int) -> np.ndarray:
    """
    Apply Fisher-Yates permutation to scramble the signal.
    Uses scatter: out[perm[i]] = signal[i].

    See EXPLANATION.md Section 5.
    """
    perm = fisher_yates_permutation(len(signal), key)
    out = np.empty_like(signal)
    out[perm] = signal  # scatter: for each i, out[perm[i]] = signal[i]
    return out


def decrypt_signal(signal: np.ndarray, key: int) -> np.ndarray:
    """
    Apply inverse permutation to unscramble the signal.
    Uses gather: out[i] = signal[perm[i]].

    See EXPLANATION.md Section 5.
    """
    perm = fisher_yates_permutation(len(signal), key)
    inv_perm = inverse_permutation(perm)
    out = np.empty_like(signal)
    out[inv_perm] = signal  # or equivalently: out = signal[perm]
    # Actually: inverse_permutation gives inv where inv[perm[i]] = i
    # To undo out[perm[i]] = x[i], we need result[i] = out[perm[i]]
    # which is: result = out[perm]
    return signal[perm]


# ═══════════════════════════════════════════════════════════════════════════
# Scaling
# ═══════════════════════════════════════════════════════════════════════════

def scale_signal(signal: np.ndarray, scale: float) -> np.ndarray:
    """Multiply signal by scale factor."""
    return signal * scale


def unscale_signal(signal: np.ndarray, scale: float) -> np.ndarray:
    """Divide signal by scale factor."""
    return signal / scale


# ═══════════════════════════════════════════════════════════════════════════
# DWT Embedding / Extraction
# ═══════════════════════════════════════════════════════════════════════════

def dwt_compute_level(carrier_len: int, target_len: int) -> int:
    """
    Compute optimal DWT level: largest L such that carrier_len / 2^L >= target_len.

    See EXPLANATION.md Section 4.
    """
    L = 0
    band_len = carrier_len
    while band_len // 2 >= target_len and L < 16:
        band_len = band_len // 2
        L += 1
    if L == 0:
        raise ValueError(f"Carrier too short ({carrier_len}) for target ({target_len})")
    return L


def dwt_cascade_embed(carrier: np.ndarray, target: np.ndarray,
                       wavelet: str = 'haar') -> tuple[np.ndarray, int]:
    """
    DWT embedding following the paper's high-pass cascade.

    The paper's flowchart: Original -> H -> H -> H ...
    At each level, we take the DETAIL (high-pass) branch.

    Implementation: use pywt for the standard multi-level decomposition.
    pywt.wavedec returns [cA_L, cD_L, cD_{L-1}, ..., cD_1].
    The paper embeds in the highest-frequency detail band (cD_1).

    Actually, re-reading the paper more carefully: the cascade goes down the
    high-pass branch at EACH level. This means:
    - Level 1: DWT(signal) -> A1, D1. Take D1.
    - Level 2: DWT(D1) -> A2, D2. Take D2.
    - ...
    This is NOT the standard wavelet packet decomposition. But for Haar,
    the result is similar to the standard decomposition's highest-frequency
    band. We implement it as iterated single-level DWT on the detail branch.

    Returns (stego_audio, level).
    """
    target_len = len(target)
    carrier_len = len(carrier)
    L = dwt_compute_level(carrier_len, target_len)

    # Pad carrier to multiple of 2^L
    pad_len = carrier_len
    remainder = carrier_len % (2 ** L)
    if remainder != 0:
        pad_len = carrier_len + (2 ** L - remainder)

    padded = np.zeros(pad_len, dtype=np.float32)
    padded[:carrier_len] = carrier

    # Cascaded DWT down the detail (high-pass) branch
    approx_bands = []
    detail_lengths = []
    current = padded.copy()

    for level in range(L):
        # Single-level DWT
        cA, cD = pywt.dwt(current, wavelet, mode='periodization')
        approx_bands.append(cA)
        detail_lengths.append(len(cD))
        current = cD  # Follow the detail branch

    # Now 'current' is the detail band at level L
    # Overwrite first target_len samples
    assert len(current) >= target_len, \
        f"Detail band length {len(current)} < target length {target_len}"
    current[:target_len] = target

    # Reconstruct back up
    for level in range(L - 1, -1, -1):
        cA = approx_bands[level]
        current = pywt.idwt(cA, current, wavelet, mode='periodization')

    # Trim to original carrier length
    stego = current[:carrier_len].astype(np.float32)
    return stego, L


def dwt_cascade_extract(stego: np.ndarray, L: int, target_len: int,
                          original_carrier_len: int,
                          wavelet: str = 'haar') -> np.ndarray:
    """
    Extract the embedded target signal from stego audio.
    Mirrors dwt_cascade_embed.
    """
    # Pad to same length as during embedding
    pad_len = original_carrier_len
    remainder = original_carrier_len % (2 ** L)
    if remainder != 0:
        pad_len = original_carrier_len + (2 ** L - remainder)

    padded = np.zeros(pad_len, dtype=np.float32)
    padded[:len(stego)] = stego

    # Cascade DWT down detail branch
    current = padded.copy()
    for level in range(L):
        _, cD = pywt.dwt(current, wavelet, mode='periodization')
        current = cD

    # Extract first target_len samples
    return current[:target_len].copy()


# ═══════════════════════════════════════════════════════════════════════════
# Quality Metrics
# ═══════════════════════════════════════════════════════════════════════════

def compute_mse(a: np.ndarray, b: np.ndarray) -> float:
    """Mean Squared Error."""
    return float(np.mean((a.astype(np.float64) - b.astype(np.float64)) ** 2))


def compute_psnr(original: np.ndarray, reconstructed: np.ndarray,
                  max_val: float = None) -> float:
    """Peak Signal-to-Noise Ratio in dB."""
    mse = compute_mse(original, reconstructed)
    if mse < 1e-20:
        return float('inf')
    if max_val is None:
        max_val = float(np.max(original))
    if max_val < 1e-20:
        max_val = 1.0
    return float(10 * np.log10(max_val ** 2 / mse))


def compute_ssim_metric(original: np.ndarray, reconstructed: np.ndarray) -> float:
    """Structural Similarity Index."""
    data_range = float(np.max(original) - np.min(original))
    if data_range < 1e-9:
        data_range = 1.0
    return float(ssim(original, reconstructed, data_range=data_range))


def compute_snr(original: np.ndarray, noisy: np.ndarray) -> float:
    """Signal-to-Noise Ratio in dB."""
    noise = noisy.astype(np.float64) - original.astype(np.float64)
    signal_pwr = float(np.mean(original.astype(np.float64) ** 2))
    noise_pwr = float(np.mean(noise ** 2))
    if noise_pwr < 1e-20:
        return float('inf')
    return float(10 * np.log10(signal_pwr / noise_pwr))


# ═══════════════════════════════════════════════════════════════════════════
# Full Pipeline
# ═══════════════════════════════════════════════════════════════════════════

def full_embed(image_path: str, carrier_path: str,
               key: int = 0xDEADBEEF, N: int = 1024,
               beta: float = 1.0, scale: float = 0.01,
               wavelet: str = 'haar',
               epsilon: float = 1e-7) -> dict:
    """
    Run the full embedding pipeline.

    Returns a dict with stego audio, header info, and intermediate signals.
    """
    import soundfile as sf

    # 1. Load image
    img, img_min, img_max = load_image(image_path)
    H, W = img.shape
    print(f"Image: {W}x{H}, range [{img_min}, {img_max}]")

    # Adjust N if needed
    if N != 2 * H:
        print(f"WARNING: N={N} != 2*H={2*H}. Using N=2*H.")
        N = 2 * H

    # 2. Image to target signal
    target = image_to_signal(img, N, beta, epsilon)
    print(f"Target signal: {len(target)} samples")

    # 3. Encrypt (permute)
    target_enc = encrypt_signal(target, key)

    # 4. Scale
    target_scaled = scale_signal(target_enc, scale)

    # 5. Load carrier
    carrier, sr = sf.read(carrier_path, dtype='float32')
    if carrier.ndim > 1:
        carrier = carrier[:, 0]  # Take channel 0 if stereo
    print(f"Carrier: {len(carrier)} samples at {sr} Hz")

    # 6. DWT embed
    stego, L = dwt_cascade_embed(carrier, target_scaled, wavelet)
    print(f"DWT level: {L}")

    # 7. Build header info
    header = {
        'magic': 0x53544547,
        'version': 1,
        'img_width': W,
        'img_height': H,
        'fft_n': N,
        'wavelet_id': 0 if wavelet == 'haar' else 1,
        'kaiser_beta': beta,
        'scale_factor': scale,
        'img_min': img_min,
        'img_max': img_max,
        'dwt_level': L,
        'original_carrier_len': len(carrier),
        'target_len': len(target),
        'key_check': key_check_hash(key),
    }

    return {
        'stego': stego,
        'header': header,
        'carrier': carrier,
        'image': img,
        'target': target,
        'target_enc': target_enc,
        'target_scaled': target_scaled,
        'sample_rate': sr,
    }


def full_extract(stego: np.ndarray, header: dict,
                  key: int = 0xDEADBEEF,
                  epsilon: float = 1e-7) -> dict:
    """
    Run the full extraction pipeline.

    Returns a dict with the reconstructed image and intermediate signals.
    """
    # Validate key
    expected_check = header['key_check']
    actual_check = key_check_hash(key)
    key_valid = (expected_check == actual_check)

    H = header['img_height']
    W = header['img_width']
    N = header['fft_n']
    L = header['dwt_level']
    scale = header['scale_factor']
    beta = header['kaiser_beta']
    target_len = header['target_len']
    carrier_len = header['original_carrier_len']
    wavelet = 'haar' if header['wavelet_id'] == 0 else 'db2'
    img_min = header['img_min']
    img_max = header['img_max']

    # 1. DWT extract
    extracted_band = dwt_cascade_extract(stego, L, target_len,
                                          carrier_len, wavelet)

    # 2. Unscale
    unscaled = unscale_signal(extracted_band, scale)

    # 3. Decrypt (inverse permutation)
    decrypted = decrypt_signal(unscaled, key)

    # 4. Signal to image
    img_recon = signal_to_image(decrypted, H, W, N, beta)

    # 5. Denormalize
    img_denorm = denormalize_image(img_recon, img_min, img_max)

    return {
        'image': img_recon,
        'image_denorm': img_denorm,
        'extracted_band': extracted_band,
        'unscaled': unscaled,
        'decrypted': decrypted,
        'key_valid': key_valid,
    }


def full_pipeline(image_path: str, carrier_path: str,
                   key: int = 0xDEADBEEF, N: int = 1024,
                   beta: float = 1.0, scale: float = 0.01,
                   wavelet: str = 'haar') -> dict:
    """
    Run full embed then extract, compute quality metrics.

    Returns a dict with all results.
    """
    # Embed
    embed_result = full_embed(image_path, carrier_path, key, N, beta, scale, wavelet)

    # Extract with correct key
    extract_result = full_extract(embed_result['stego'], embed_result['header'], key)

    # Extract with wrong key (should give noise)
    wrong_key = key ^ 0x12345678
    extract_wrong = full_extract(embed_result['stego'], embed_result['header'], wrong_key)

    # Compute metrics
    original_img = embed_result['image']
    recon_img = extract_result['image']
    wrong_img = extract_wrong['image']
    original_carrier = embed_result['carrier']
    stego_audio = embed_result['stego']

    results = {
        'embed': embed_result,
        'extract': extract_result,
        'extract_wrong_key': extract_wrong,
        'metrics': {
            'image_mse': compute_mse(original_img, recon_img),
            'image_psnr': compute_psnr(original_img, recon_img, max_val=1.0),
            'image_ssim': compute_ssim_metric(original_img, recon_img),
            'audio_snr': compute_snr(original_carrier, stego_audio),
            'audio_mse': compute_mse(original_carrier, stego_audio),
            'wrong_key_mse': compute_mse(original_img, wrong_img),
            'wrong_key_psnr': compute_psnr(original_img, wrong_img, max_val=1.0),
        },
    }

    # Print results
    print("\n" + "=" * 60)
    print("PIPELINE RESULTS")
    print("=" * 60)
    for k, v in results['metrics'].items():
        print(f"  {k:20s}: {v:.6f}")
    print("=" * 60)

    return results


# ═══════════════════════════════════════════════════════════════════════════
# CLI
# ═══════════════════════════════════════════════════════════════════════════

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='DSP Steganography Reference Pipeline')
    parser.add_argument('--image', '-i', help='Input grayscale image path')
    parser.add_argument('--carrier', '-c', help='Input carrier WAV path')
    parser.add_argument('--key', '-k', type=lambda x: int(x, 0), default=0xDEADBEEF,
                        help='Encryption key (hex or decimal)')
    parser.add_argument('--beta', type=float, default=1.0, help='Kaiser beta')
    parser.add_argument('--scale', type=float, default=0.01, help='Embedding scale')
    parser.add_argument('--wavelet', default='haar', help='Wavelet (haar or db2)')
    parser.add_argument('--selftest', action='store_true',
                        help='Run self-test with synthetic data')
    args = parser.parse_args()

    if args.selftest:
        # Self-test with synthetic data
        print("Running self-test with synthetic 8x8 image and random carrier...")
        np.random.seed(42)

        # Create small test image
        test_img = np.random.rand(8, 8).astype(np.float32)
        N = 16  # 2 * 8

        # Image to signal
        target = image_to_signal(test_img, N, beta=1.0)
        print(f"Target signal length: {len(target)}")

        # Encrypt
        target_enc = encrypt_signal(target, key=42)

        # Scale
        target_scaled = scale_signal(target_enc, 0.01)

        # Create random carrier
        carrier = np.random.randn(4096).astype(np.float32)

        # Embed
        stego, L = dwt_cascade_embed(carrier, target_scaled, 'haar')
        print(f"DWT level: {L}")

        # Extract
        extracted = dwt_cascade_extract(stego, L, len(target), len(carrier), 'haar')
        unscaled = unscale_signal(extracted, 0.01)
        decrypted = decrypt_signal(unscaled, key=42)
        recon_img = signal_to_image(decrypted, 8, 8, N, beta=1.0)

        # Metrics
        mse = compute_mse(test_img, recon_img)
        psnr = compute_psnr(test_img, recon_img, max_val=1.0)
        print(f"\nSelf-test results:")
        print(f"  MSE:  {mse:.10f}")
        print(f"  PSNR: {psnr:.2f} dB")

        # Wrong key test
        wrong_dec = decrypt_signal(unscaled, key=9999)
        wrong_img = signal_to_image(wrong_dec, 8, 8, N, beta=1.0)
        wrong_mse = compute_mse(test_img, wrong_img)
        print(f"  Wrong key MSE: {wrong_mse:.6f} (should be large)")

        if mse < 1e-4:
            print("\n✓ SELF-TEST PASSED")
        else:
            print(f"\n✗ SELF-TEST FAILED (MSE={mse} too high)")
            sys.exit(1)

    elif args.image and args.carrier:
        results = full_pipeline(args.image, args.carrier, args.key,
                                 beta=args.beta, scale=args.scale,
                                 wavelet=args.wavelet)
    else:
        parser.print_help()
        print("\nUse --selftest for a quick self-test, or provide --image and --carrier.")
