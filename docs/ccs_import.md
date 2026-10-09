# Importing and Debugging in Code Composer Studio (CCS)

This guide covers importing the pre-configured project, connecting your XDS100v3 debug probe, loading data, executing, and graphing signals.

## 1. Import Project into CCS

1. Clone or pull the repository from GitHub onto your local host machine.
2. In CCS, select **File > Import...**
3. Choose **Code Composer Studio > CCS Projects**, then click **Next**.
4. Set **Select search-directory** to `<repo_root>/dsp_stego/target`.
5. In the discovered projects box, check `dsp_stego`.
6. **CRITICAL**: Ensure **"Copy projects into workspace" is UNCHECKED**. Leaving it unchecked maintains git repository tracking.
7. Click **Finish**.

## 2. Target Configuration & GEL Setup

1. Locate [C6748_XDS100v3.ccxml.template](file:///home/moham/projects/steganography_image_dsp/dsp_stego/target/C6748_XDS100v3.ccxml.template). Save as `C6748_XDS100v3.ccxml`.
2. Double-click the `.ccxml` file.
3. In the Basic tab:
   - Connection: **Texas Instruments XDS100v3 USB Debug Probe**
   - Device: **TMS320C6748**
4. In Advanced tab:
   - Expand `C674X_0`.
   - Verify the initialization script links to your board's GEL file (`C6748_LCDK.gel` or custom board init script).
5. Click **Save** and **Test Connection**.

## 3. Build & Load

1. Right-click the project > **Build Project**. Ensure build succeeds and produces `build_target/stego.out` (or `Debug/stego.out`).
2. Right-click `C6748_XDS100v3.ccxml` > **Launch Selected Configuration**.
3. In Debug View, right-click `Texas Instruments XDS100v3 USB Debug Probe/C674X_0` > **Connect Target**.
   *(Watch the Console: GEL script should output PLL and DDR2 initialization logs).*
4. Select **Run > Load > Load Program...** and choose your `.out` binary.

## 4. Visualizing Signals with CCS Graph Tool

The project places signals at fixed DDR2 addresses that can be inspected directly in real-time:
- **Target Signal (Scrambled Time-Domain)**: `0xC0200000` (length 524,288 float32)
- **Carrier Audio Input**: `0xC0400000` (float32)
- **Stego Audio Output**: `0xC1400000` (float32)

To view:
1. Go to **Tools > Graph > Single Time**.
2. Set configuration:
   - **Start Address**: `0xC0200000`
   - **Acquisition Buffer Size**: `1024` (or `2048`)
   - **Display Data Size**: `1024`
   - **DSP Data Type**: `32-bit floating point`
   - **Sampling Rate (Hz)**: `44100`
3. Click **OK** to observe time-domain waveforms and spectral energy distribution.

## 5. Troubleshooting Guide

- **DDR2 Bus Errors / Memory Read Failures (0xC0000000 accessible fails)**:
  - *Cause*: DDR2 controller clock or PLL was not initialized before loading code.
  - *Fix*: Re-run the GEL script via **Scripts > C6748 System Initialization > Init_DDR2** in the CCS menu.
- **Wrong Compiler Version Warning**:
  - *Cause*: Installed TI CGT version differs from project default (e.g., v8.3.x vs v7.4.x).
  - *Fix*: Go to **Properties > General > Compiler version** and select your installed compiler toolchain.
- **Line Ending Issues (`^M` carriage returns in bash/scripts)**:
  - Repository uses `.gitattributes` (`* text=auto`). Run `dos2unix` on shell scripts if running under Linux/WSL.
