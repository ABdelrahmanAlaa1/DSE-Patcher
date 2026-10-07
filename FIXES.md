
# DSE-Patcher Source Code Fixes

## Bugs Found & Fixed

### 1. **CRITICAL: `-enable` hardcodes `0x6`, destroys system flags** (line 2277)
- `dwDSEEnableValue = 6` overwrites ALL flags including `0x2000` (FLIGHTSIGNING), `0x8` (TESTSIGN), etc.
- **Fix**: `-enable` should restore the *captured original value*, not a hardcoded `0x6`

### 2. **CRITICAL: `-restore` captures wrong value** (line 2349)
- It reads g_CiOptions *at CLI startup* as the "original" value
- If g_CiOptions was already modified (e.g., by a previous broken -enable), the "original" is wrong
- **Fix**: Save original value to a file on first disable, use that for restore

### 3. **CRITICAL: `-auto` has no sleep** (lines 2414-2443)
- Disables DSE then immediately restores — zero usable time window
- **Fix**: Parse `-auto:<seconds>`, sleep for that duration between disable and restore

### 4. **CRITICAL: `-auto:<seconds>` not parsed** (line 619)
- `_stricmp(lpCmdLine, "-auto")` requires exact match; `-auto:15` goes to unknown arg
- **Fix**: Use `_strnicmp` prefix match + parse seconds after colon

### 5. **HIGH: `DEFAULT_DRIVER_INDEX = 4`** (line 2198)
- Index 4 = DBUtil v2.7 — may not work on all systems
- Index 0 = RTCore64 — more widely compatible
- **Fix**: Change to 0, or add `-driver:N` CLI arg

### 6. **HIGH: No restore verification/retry** (lines 2405-2412)
- If restore write fails, DSE stays disabled — potential GSOD on some configs
- **Fix**: Re-read after write, retry once if mismatch, warn if still wrong

### 7. **MEDIUM: No recovery path in cleanup** (line 2473)
- On error, cleanup doesn't attempt to restore original value
- **Fix**: In cleanup, attempt emergency restore if DSE was modified
