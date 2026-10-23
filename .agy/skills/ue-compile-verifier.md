# Skill: ue-compile-verifier
Description: Autonomous compilation verification using UnrealBuildTool (UBT) for Dedicated Server and Editor targets.

## When to Execute
Run this skill automatically after modifying any C++ header (`.h`) or source (`.cpp`) files to ensure zero compilation and linkage errors before concluding a task.

## Execution Procedure

### Step 1: Compile Dedicated Server Target
Execute via terminal:
```cmd
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" WildCardServer Win64 Development -Project="${workspaceFolder}\WildCard.uproject" -WaitMutex -FromMsBuild
```

### Step 2: Compile Editor Target

Execute via terminal:

```cmd
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" WildCardEditor Win64 Development -Project="${workspaceFolder}\WildCard.uproject" -WaitMutex -FromMsBuild
```

### Step 3: Parse and Self-Correct

* If the build returns exit code `0`: Output compilation success.
* If the build fails:
1. Capture the exact error codes (e.g., `C2065`, `C2664`, `LNK2019`, `UHT error`).
2. Locate the offending file path and line number.
3. Analyze whether the error is due to missing headers, unresolved UHT macros, incorrect `TObjectPtr` usage, server/client symbol mismatches, or misuse of `WithValidation` on `Client` RPCs.
4. Edit the source code to resolve the error.
5. Re-run Step 1 and Step 2 until both targets build cleanly.
