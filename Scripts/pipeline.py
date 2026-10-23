#!/usr/bin/env python3
"""
WildCard Project Pipeline Runner (Scripts/pipeline.py)
Autonomous pipeline CLI supporting:
  - inspect: Fast/Deep asset metadata inspection (BP, WBP, Anim, uasset)
  - compile: Compile Dedicated Server and Editor targets via UBT
  - test: Headless RunUAT automation tests
  - all: Run compile and test
"""

import sys
import os
import json
import subprocess
import argparse
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
UBT_BAT = Path(r"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat")
RUNUAT_BAT = Path(r"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat")
UPROJECT_FILE = PROJECT_ROOT / "WildCard.uproject"

def run_inspect_check(asset: str, deep: bool = False, emit_json: bool = False) -> int:
    """Runs dump_asset_metadata.py and prints structured summary or raw JSON."""
    if not asset:
        print("[ERROR] --asset argument is required for 'inspect' mode.")
        print("Usage: python Scripts/pipeline.py inspect --asset <path_or_game_path>")
        return 1

    script_path = (PROJECT_ROOT / "Scripts" / "dump_asset_metadata.py").resolve()
    cmd = [sys.executable, str(script_path), "--asset", asset, "--json"]
    if deep:
        cmd.append("--deep")

    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=str(PROJECT_ROOT))
    if proc.returncode != 0:
        print(f"[ERROR] Asset inspection failed with return code {proc.returncode}:")
        if proc.stderr:
            print(proc.stderr.strip())
        if proc.stdout:
            print(proc.stdout.strip())
        return proc.returncode

    try:
        data = json.loads(proc.stdout.strip())
    except json.JSONDecodeError:
        print(f"[ERROR] Failed to parse JSON output from metadata dumper:\n{proc.stdout}")
        return 1

    if emit_json:
        print(json.dumps(data, indent=2, ensure_ascii=False))
        return 0

    # Print Compact, Human-Readable Ground Truth Summary
    print("=" * 80)
    print(f" [ASSET METADATA INSPECTION] - {data.get('asset_name', 'Unknown')}")
    print("=" * 80)
    print(f"  Asset Type       : {data.get('asset_type', 'Unknown')}")
    print(f"  Inspection Mode  : {data.get('inspection_mode', 'N/A')}")
    print(f"  Game Path        : {data.get('game_path', 'N/A')}")
    print(f"  File Path        : {data.get('file_path', 'N/A')}")
    print(f"  Valid UAsset     : {data.get('is_valid_uasset', False)}")
    if "file_size_bytes" in data:
        print(f"  File Size        : {data['file_size_bytes']:,} bytes")
    print("-" * 80)

    # Widgets (WBP)
    if "detected_widgets" in data and data["detected_widgets"]:
        print(f"  * Detected Widgets ({len(data['detected_widgets'])}):")
        for w in data["detected_widgets"][:15]:
            print(f"      - {w}")
        if len(data["detected_widgets"]) > 15:
            print(f"      ... and {len(data['detected_widgets']) - 15} more")

    # ViewModels (WBP)
    if "detected_viewmodels" in data and data["detected_viewmodels"]:
        print(f"  * Detected ViewModels ({len(data['detected_viewmodels'])}):")
        for vm in data["detected_viewmodels"]:
            print(f"      - {vm}")

    # Bindings (WBP)
    if "detected_bindings" in data and data["detected_bindings"]:
        print(f"  * Detected Bindings ({len(data['detected_bindings'])}):")
        for b in data["detected_bindings"]:
            print(f"      - {b}")

    # Notifies (Anim)
    if "detected_notifies" in data and data["detected_notifies"]:
        print(f"  * Detected Notifies ({len(data['detected_notifies'])}):")
        for n in data["detected_notifies"]:
            print(f"      - {n}")

    # Slots (Anim)
    if "detected_slots" in data and data["detected_slots"]:
        print(f"  * Detected Slots ({len(data['detected_slots'])}):")
        for s in data["detected_slots"]:
            print(f"      - {s}")

    # Tracks (Anim)
    if "detected_tracks" in data and data["detected_tracks"]:
        print(f"  * Detected Tracks ({len(data['detected_tracks'])}):")
        for t in data["detected_tracks"]:
            print(f"      - {t}")

    # Components (BP)
    if "detected_components" in data and data["detected_components"]:
        print(f"  * Detected Components ({len(data['detected_components'])}):")
        for c in data["detected_components"]:
            print(f"      - {c}")

    # Variables (BP)
    if "detected_variables" in data and data["detected_variables"]:
        print(f"  * Detected Variables ({len(data['detected_variables'])}):")
        for v in data["detected_variables"]:
            print(f"      - {v}")

    # Gameplay Tags
    if "detected_gameplay_tags" in data and data["detected_gameplay_tags"]:
        print(f"  * Detected GameplayTags ({len(data['detected_gameplay_tags'])}):")
        for tag in data["detected_gameplay_tags"]:
            print(f"      - {tag}")

    # Referenced Engine Classes
    if "referenced_engine_classes" in data and data["referenced_engine_classes"]:
        print(f"  * Referenced Engine / Native Classes:")
        for ec in data["referenced_engine_classes"]:
            print(f"      - {ec}")

    print("=" * 80)
    return 0

def run_compile(target: str = "both") -> int:
    """Compiles Dedicated Server and/or Editor targets using UBT."""
    targets = []
    if target in ("server", "both"):
        targets.append("WildCardServer")
    if target in ("editor", "both"):
        targets.append("WildCardEditor")

    if not UBT_BAT.exists():
        print(f"[ERROR] UBT not found at {UBT_BAT}")
        return 1

    for tgt in targets:
        print(f"\n[COMPILE] Building target: {tgt} Win64 Development...")
        cmd = [
            str(UBT_BAT),
            tgt,
            "Win64",
            "Development",
            f"-Project={UPROJECT_FILE}",
            "-WaitMutex",
            "-FromMsBuild"
        ]
        proc = subprocess.run(cmd, cwd=str(PROJECT_ROOT))
        if proc.returncode != 0:
            print(f"[RETRY] Retrying compilation with -LiveCoding for {tgt}...")
            cmd_live = cmd + ["-LiveCoding"]
            proc = subprocess.run(cmd_live, cwd=str(PROJECT_ROOT))
            if proc.returncode != 0:
                print(f"[FAIL] Compilation failed for {tgt} with code {proc.returncode}")
                return proc.returncode
        print(f"[PASS] Successfully compiled {tgt}")

    return 0

def run_automation_tests() -> int:
    """Runs headless automation tests via RunUAT."""
    if not RUNUAT_BAT.exists():
        print(f"[ERROR] RunUAT not found at {RUNUAT_BAT}")
        return 1

    print("\n[TEST] Running Card Game Automation Tests via RunUAT...")
    cmd = [
        str(RUNUAT_BAT),
        "RunTests",
        f"-project={UPROJECT_FILE}",
        "-test=WildCard.CardGame",
        "-nullrhi",
        "-nosound",
        "-unattended"
    ]
    proc = subprocess.run(cmd, cwd=str(PROJECT_ROOT))
    if proc.returncode != 0:
        print(f"[FAIL] Automation tests failed with code {proc.returncode}")
        return proc.returncode
    print("[PASS] Automation tests passed.")
    return 0

def main():
    parser = argparse.ArgumentParser(description="WildCard Project Pipeline Runner")
    parser.add_argument(
        "mode",
        choices=["inspect", "compile", "build", "test", "all"],
        help="Pipeline mode to execute"
    )
    parser.add_argument("--asset", "-a", type=str, help="Asset file path or /Game/ path for inspect mode")
    parser.add_argument("--deep", action="store_true", help="Run deep engine reflection via UnrealEditor-Cmd")
    parser.add_argument("--json", action="store_true", help="Emit raw JSON in inspect mode")
    parser.add_argument("--target", choices=["server", "editor", "both"], default="both", help="Target for compile mode")

    args = parser.parse_args()

    if args.mode == "inspect":
        rc = run_inspect_check(args.asset, deep=args.deep, emit_json=args.json)
        sys.exit(rc)
    elif args.mode in ("compile", "build"):
        rc = run_compile(target=args.target)
        sys.exit(rc)
    elif args.mode == "test":
        rc = run_automation_tests()
        sys.exit(rc)
    elif args.mode == "all":
        rc = run_compile(target="both")
        if rc != 0:
            sys.exit(rc)
        rc = run_automation_tests()
        sys.exit(rc)

if __name__ == "__main__":
    main()
