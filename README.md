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
  -disable   Disable Driver Signature Enforcement
  -enable    Enable Driver Signature Enforcement
  -restore   Restore DSE to the value captured at CLI startup
  -help      Show help message
```

If no arguments are provided, the GUI will be launched.

**Notes:**
- This tool requires Administrator privileges.
- CLI mode uses the RTCore64 driver for kernel memory access.

**PowerShell 7 Users:** Due to how PowerShell 7 handles GUI applications, you may need to use one of these methods to see CLI output:
```powershell
# Method 1: Use cmd.exe to run the command
cmd /c DSE-Patcher.exe -help

# Method 2: Use Start-Process with -Wait
Start-Process -FilePath "DSE-Patcher.exe" -ArgumentList "-disable" -Wait -NoNewWindow
```
