<h1>DSE-Patcher - Windows 11</h1>
<p>Originally based from https://github.com/gmh5225/DSE-Patcher but with improvements and support for Windows 11 home.</p>

Note: Make sure you have disabled this option:
![image](https://github.com/user-attachments/assets/59c34305-6918-4e87-bb12-d14061696098)

# DSE-Patcher
https://www.codeproject.com/Articles/5348168/Disable-Driver-Signature-Enforcement-with-DSE-Patc

## Command Line Interface

DSE-Patcher supports command line arguments for scripting and automation:

```
Usage: DSE-Patcher.exe [options]

Options:
  -disable        Disable Driver Signature Enforcement
                  Saves original g_CiOptions value for reliable restore
  -enable         Enable/restore DSE (uses saved original value)
                  Preserves system flags like flightsigning (0x2000)
  -restore        Restore DSE to saved original value with verification
  -auto:<seconds> Disable DSE, wait N seconds, then restore
                  Example: -auto:15
  -help           Show help message
```

If no arguments are provided, the GUI will be launched.

**Notes:**
- This tool requires Administrator privileges.
- CLI mode defaults to RTCore64 driver (index 0) for kernel memory access.
- Original g_CiOptions value is saved to `DSE-Patcher.dse_saved` alongside the executable.
- `-enable` and `-restore` use the saved value to preserve system flags (e.g., `0x2006` instead of hardcoded `0x6`).
- All restore operations include verification (re-read after write) and retry on failure.
- Emergency restore is attempted on any error path to prevent leaving DSE disabled.

## Compatibility
- Windows 11 24H2 (Build 26100) and 26H2 (Build 26xxx) fully supported.
- Windows Vista through Windows 11 23H2 supported (legacy paths).
