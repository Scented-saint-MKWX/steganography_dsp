# CCS Project Creation Guide (Bootstrap)

Because TI Code Composer Studio (CCS) relies on proprietary project configurations (`.project`, `.cproject`, `.ccsproject`) tied to toolchain installations, generate the base project directly inside CCS once following these exact steps.

## Step-by-Step Project Creation in CCS (v12.x+)

1. **Launch CCS**: Open Code Composer Studio and select an empty workspace (or your regular workspace).
2. **New CCS Project**:
   - Navigate to **File > New > CCS Project**.
   - **Target**: Select `TMS320C6748`.
   - **Connection**: Select `Texas Instruments XDS100v3 USB Debug Probe`.
   - **Project name**: Enter `dsp_stego`.
   - **Location**: **Uncheck** "Use default location". Browse and select the repository directory:
     `<repo_path>/dsp_stego/target`
   - **Compiler version**: Select `TI v8.3.x` (or newer installed C6000 CGT).
   - **Project-templates and examples**: Choose **Empty Project (with main.c)**. (CCS will detect our existing `main.c`).
   - Click **Finish**.

3. **Configure Compiler & Linker Settings**:
   - Right-click project in Project Explorer > **Properties**.
   - Under **CCS Build > C6000 Compiler > Processor Options**:
     - Silicon version (`-mv`): `6740` (`-mv6740`)
     - Floating point support (`--float_support`): `fpu32`
   - Under **CCS Build > C6000 Compiler > Include Options**:
     - Add: `${PROJECT_ROOT}`
     - Add: `${PROJECT_ROOT}/core`
     - Add: `${PROJECT_ROOT}/io`
     - Add: `${PROJECT_ROOT}/util`
     *(Note: Always use `${PROJECT_ROOT}` instead of hardcoded absolute paths!)*
   - Under **CCS Build > C6000 Linker > File Search Path**:
     - Under "Include library-file or command-file as input": add `memmap.cmd`.
     - Add libc runtime: `rts6740_elf.lib`.

4. **Verify Target Configuration**:
   - If `C6748_XDS100v3.ccxml` is present, right-click it and choose **Set as Active Target Configuration**.
   - In target configuration editor, ensure the board GEL file (e.g., `C6748_LCDK.gel`) is configured for the C674X DSP core to initialize PLL and DDR2 on connect.

5. **Commit the Resulting Files**:
   After creating and verifying the project build, commit only:
   - `target/.project`
   - `target/.cproject`
   - `target/.ccsproject`
   - `target/C6748_XDS100v3.ccxml`

   **DO NOT commit**:
   - `Debug/`, `Release/`, `.settings/`, `*.obj`, `*.out`, `*.map`.
