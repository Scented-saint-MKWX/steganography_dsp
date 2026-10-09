#!/usr/bin/env python3
"""
run_all.py - Complete end-to-end automation runner

Supports:
  --backend host  : Executes C reference stego host test or Python reference pipeline locally
  --backend board : Calls TI DSS (Debug Server Scripting) to run on TMS320C6748 hardware

Workflow:
  1. Prepare inputs (image/audio to float32 .bin)
  2. Execute execution pipeline (host native or target DSP via DSS)
  3. Postprocess outputs and evaluate metrics (PSNR, SSIM, SNR)
"""

import argparse
import os
import sys
import subprocess
import shutil

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def run_cmd(cmd, cwd=PROJECT_ROOT):
    print(f"[RUN] {cmd}")
    res = subprocess.run(cmd, shell=True, cwd=cwd)
    if res.returncode != 0:
        print(f"[ERROR] Command failed with code {res.returncode}")
        sys.exit(res.returncode)

def main():
    parser = argparse.ArgumentParser(description="End-to-end Steganography Runner")
    parser.add_argument("--backend", choices=["host", "board"], default="host",
                        help="Execution backend: 'host' (local C/Python) or 'board' (DSS on C6748)")
    parser.add_argument("--image", default=None, help="Input grayscale image")
    parser.add_argument("--carrier", default=None, help="Input carrier WAV")
    parser.add_argument("--selftest", action="store_true", help="Run full suite self-test")
    parser.add_argument("--key", default="0xDEADBEEF", help="Key for permutation")
    args = parser.parse_args()

    python_bin = sys.executable

    if args.backend == "host":
        print("=== Running Host Verification ===")
        # 1. Run unit tests
        run_cmd("target/build_host/test_runner")
        
        # 2. Run Python self-test
        run_cmd(f"{python_bin} host/ref_pipeline.py --selftest")
        
        # 3. Run C stego host application
        run_cmd("target/build_host/stego_host")
        print("\n[SUCCESS] Host verification executed successfully!")

    elif args.backend == "board":
        print("=== Running TMS320C6748 Board Execution via DSS ===")
        dss_exec = shutil.which("dss.sh") or shutil.which("dss.bat") or shutil.which("dss")
        if not dss_exec:
            print("[ERROR] TI DSS ('dss') not found on PATH. Ensure CCS scripting environment is configured.")
            print("Refer to docs/ccs_import.md and scripts/bootstrap_ccs.md for setup instructions.")
            sys.exit(1)

        run_cmd(f"{dss_exec} host/run_target.js {'selftest' if args.selftest else 'embed'}")
        print("\n[SUCCESS] Board execution completed via DSS!")

if __name__ == "__main__":
    main()
