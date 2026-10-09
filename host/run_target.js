/**
 * run_target.js - Debug Server Scripting (DSS) Automation Script for TMS320C6748
 *
 * Automates:
 *   1. Connecting to TMS320C6748 via XDS100v3 emulator
 *   2. Initializing target through GEL (PLL & DDR2 setup)
 *   3. Loading stego.out binary to DSP memory
 *   4. Writing input files (Image, Carrier, Header) into DDR2 addresses
 *   5. Running the program and polling the done flag with a timeout
 *   6. Saving output stego audio, recovered image, and results structure to disk
 *   7. Disconnecting cleanly
 *
 * Usage:
 *   dss run_target.js [embed|extract|selftest]
 */

// Import DSS packages
importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.dss.symbolManager);
importPackage(Packages.java.lang);

// Configuration paths (Adjust CCXML and target binary paths as needed)
var CCXML_FILE = "../target/C6748_XDS100v3.ccxml";
var OUT_FILE   = "../target/build_target/stego.out";

// Fixed DDR2 Addresses from config.h / memmap.cmd
var DDR2_IMAGE_IN_ADDR  = 0xC0000000;
var DDR2_CARRIER_ADDR   = 0xC0400000;
var DDR2_STEGO_ADDR     = 0xC1400000;
var DDR2_IMAGE_OUT_ADDR = 0xC2400000;
var DDR2_HEADER_ADDR    = 0xC2800000;
var DDR2_RESULTS_ADDR   = 0xC2800100;
var DDR2_DONE_FLAG_ADDR = 0xC2800200;

var POLL_TIMEOUT_MS = 60000; // 60 seconds

// Create Scripting Environment
var script = ScriptingEnvironment.instance();
script.setScriptTimeout(120000);

print("=== Starting DSS Automation for TMS320C6748 ===");

// Get Debug Server
var debugServer = script.getServer("DebugServer.1");
debugServer.setConfig(CCXML_FILE);

var debugSession = debugServer.openSession(".*C674X_0.*");
print("Connecting to target CPU...");
debugSession.target.connect();

// Reset target and load program
print("Resetting target and loading: " + OUT_FILE);
debugSession.target.reset();
debugSession.memory.loadProgram(OUT_FILE);

// Set done flag to 0 before execution
debugSession.memory.writeData(0, DDR2_DONE_FLAG_ADDR, 0, 32);

// Check optional command-line argument for mode
var mode = "selftest";
if (arguments.length > 0) {
    mode = arguments[0];
}

if (mode === "embed") {
    print("Loading image and carrier raw inputs to DDR2...");
    debugSession.memory.loadRaw(0, DDR2_IMAGE_IN_ADDR, "../data/input_image.bin", 32, false);
    debugSession.memory.loadRaw(0, DDR2_CARRIER_ADDR,  "../data/input_carrier.bin", 32, false);
} else if (mode === "extract") {
    print("Loading stego audio and header to DDR2...");
    debugSession.memory.loadRaw(0, DDR2_STEGO_ADDR,  "../data/stego_audio.bin", 32, false);
    debugSession.memory.loadRaw(0, DDR2_HEADER_ADDR, "../data/stego.hdr", 8, false);
}

// Run target asynchronously
print("Running target execution...");
debugSession.target.runAsynch();

// Poll done flag
var startTime = java.lang.System.currentTimeMillis();
var done = 0;
while (done === 0) {
    java.lang.Thread.sleep(500);
    done = debugSession.memory.readData(0, DDR2_DONE_FLAG_ADDR, 32);
    if ((java.lang.System.currentTimeMillis() - startTime) > POLL_TIMEOUT_MS) {
        print("ERROR: Target execution timed out waiting for done flag!");
        debugSession.target.halt();
        break;
    }
}

if (done === 1) {
    print("Target execution completed successfully!");
    
    // Dump results struct
    print("Dumping results struct from 0x" + java.lang.Long.toHexString(DDR2_RESULTS_ADDR) + "...");
    debugSession.memory.saveRaw(0, DDR2_RESULTS_ADDR, "../docs/results/board_results.bin", 256, 32, false);
    
    if (mode === "embed" || mode === "selftest") {
        print("Saving stego audio output...");
        debugSession.memory.saveRaw(0, DDR2_STEGO_ADDR, "../data/output_stego.bin", 131072 * 4, 32, false);
    }
    if (mode === "extract" || mode === "selftest") {
        print("Saving extracted image output...");
        debugSession.memory.saveRaw(0, DDR2_IMAGE_OUT_ADDR, "../data/output_extracted.bin", 64 * 64 * 4, 32, false);
    }
}

// Cleanup and disconnect
print("Disconnecting and closing debug session...");
debugSession.target.disconnect();
debugServer.stop();
print("=== DSS Automation Finished ===");
