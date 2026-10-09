import numpy as np
import os
import json
import scipy.signal
import scipy.fft
import pywt
from prng import xorshift32, fisher_yates_permutation, inverse_permutation
from ref_pipeline import make_pair, istft_no_overlap

def gen_prng_vectors(out_dir, manifest):
    keys = [1, 0xDEADBEEF, 0x12345678]
    files = []
    for k in keys:
        state = k
        outs = []
        for _ in range(100):
            state, val = xorshift32(state)
            outs.append(val)
        out_arr = np.array(outs, dtype=np.uint32)
        fname = f"prng_k{k}.bin"
        out_arr.tofile(os.path.join(out_dir, fname))
        files.append({'file': fname, 'type': 'prng', 'key': k})
    manifest['prng'] = files

def gen_perm_vectors(out_dir, manifest):
    lengths = [16, 256, 1024]
    keys = [1, 42, 0xDEADBEEF]
    files = []
    for L in lengths:
        for k in keys:
            perm = fisher_yates_permutation(L, k)
            inv = inverse_permutation(perm)
            
            pname = f"perm_L{L}_k{k}.bin"
            iname = f"iperm_L{L}_k{k}.bin"
            
            perm.tofile(os.path.join(out_dir, pname))
            inv.tofile(os.path.join(out_dir, iname))
            
            files.append({'perm_file': pname, 'inv_file': iname, 'length': L, 'key': k})
    manifest['permutation'] = files

def gen_kaiser_vectors(out_dir, manifest):
    Ns = [64, 256, 1024]
    betas = [0, 1, 5, 14]
    files = []
    for N in Ns:
        for b in betas:
            w = scipy.signal.windows.kaiser(N, b).astype(np.float32)
            fname = f"kaiser_N{N}_b{b}.bin"
            w.tofile(os.path.join(out_dir, fname))
            files.append({'file': fname, 'N': N, 'beta': b})
    manifest['kaiser'] = files

def gen_fft_vectors(out_dir, manifest):
    Ns = [16, 64, 256, 1024]
    files = []
    for N in Ns:
        inp = np.random.rand(N).astype(np.float32)
        out = scipy.fft.fft(inp).astype(np.complex64)
        
        # Interleave real and imag for output
        out_interleaved = np.zeros(2*N, dtype=np.float32)
        out_interleaved[0::2] = np.real(out)
        out_interleaved[1::2] = np.imag(out)
        
        iname = f"fft_in_N{N}.bin"
        oname = f"fft_out_N{N}.bin"
        inp.tofile(os.path.join(out_dir, iname))
        out_interleaved.tofile(os.path.join(out_dir, oname))
        
        files.append({'in_file': iname, 'out_file': oname, 'N': N})
    manifest['fft'] = files

def main():
    out_dir = '../data/vectors'
    os.makedirs(out_dir, exist_ok=True)
    manifest = {}
    
    gen_prng_vectors(out_dir, manifest)
    gen_perm_vectors(out_dir, manifest)
    gen_kaiser_vectors(out_dir, manifest)
    gen_fft_vectors(out_dir, manifest)
    
    with open(os.path.join(out_dir, 'manifest.json'), 'w') as f:
        json.dump(manifest, f, indent=4)
        
    print(f"Generated test vectors in {out_dir}")

if __name__ == '__main__':
    main()
