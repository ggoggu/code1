#!/usr/bin/env python3
"""
Unreal Engine 5 Asset Metadata Dumper (Scripts/dump_asset_metadata.py)
Extracts structured JSON metadata from BP, WBP, Animation, and other .uasset files.
"""

import sys
import os
import re
import json
import subprocess
import argparse
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
CONTENT_DIR = PROJECT_ROOT / "Content"

# Project .uproject auto-discovery
uproject_candidates = list(PROJECT_ROOT.glob("*.uproject"))
UPROJECT_FILE = uproject_candidates[0] if uproject_candidates else None

try:
    import unreal
    IN_ENGINE = True
except ImportError:
    unreal = None
    IN_ENGINE = False

def resolve_asset_paths(asset_arg: str) -> tuple[Path | None, str | None]:
    clean_arg = asset_arg.strip().replace("\\", "/")
    if clean_arg.startswith("/Game/"):
        rel_sub = clean_arg[len("/Game/"):]
        if "." in rel_sub:
            rel_sub = rel_sub.split(".")[0]
        file_path = CONTENT_DIR / f"{rel_sub}.uasset"
        game_path = f"/Game/{rel_sub}"
        return file_path if file_path.exists() else None, game_path
        
    p = Path(clean_arg)
    if not p.is_absolute():
        candidate1 = PROJECT_ROOT / p
        candidate2 = CONTENT_DIR / p
        if candidate1.exists() or candidate1.with_suffix(".uasset").exists():
            p = candidate1
        elif candidate2.exists() or candidate2.with_suffix(".uasset").exists():
            p = candidate2
        else:
            p = candidate1

    if not p.suffix:
        p = p.with_suffix(".uasset")
    if not p.exists():
        # Fallback: search for file name in Content directory
        matches = list(CONTENT_DIR.rglob(f"{p.stem}.uasset"))
        if matches:
            p = matches[0]
        else:
            return None, None
    try:
        rel = p.resolve().relative_to(CONTENT_DIR.resolve())
        game_path = f"/Game/{rel.with_suffix('').as_posix()}"
    except ValueError:
        game_path = None
    return p, game_path

def parse_uasset_binary_fast(file_path: Path, game_path: str | None) -> dict:
    file_size = file_path.stat().st_size
    with open(file_path, "rb") as f:
        raw = f.read()

    is_valid_package = len(raw) >= 4 and raw[:4] == b'\xc1\x83\x2a\x9e'
    string_matches = re.findall(b'[\x20-\x7E]{3,}', raw)
    strings = [s.decode('utf-8', 'ignore') for s in string_matches]

    asset_type = "Unknown"
    if any("WidgetTree" in s or "ModelViewViewModel" in s for s in strings):
        asset_type = "WidgetBlueprint"
    elif any("AnimMontage" in s or "CompositeSection" in s for s in strings):
        asset_type = "AnimMontage"
    elif any("AnimBlueprint" in s or "AnimGraph" in s for s in strings):
        asset_type = "AnimBlueprint"
    elif any("BlendSpace" in s for s in strings):
        asset_type = "BlendSpace"
    elif any("AnimSequence" in s or "AnimationData" in s for s in strings):
        asset_type = "AnimSequence"
    elif any("DataTable" in s for s in strings):
        asset_type = "DataTable"
    elif any("SimpleConstructionScript" in s or "BlueprintGeneratedClass" in s for s in strings):
        asset_type = "Blueprint"

    parent_classes = []
    for s in strings:
        if s.startswith("/Script/") and "." in s:
            cls_name = s.split(".")[-1]
            if cls_name not in parent_classes and not cls_name.startswith("Default__"):
                parent_classes.append(cls_name)

    metadata = {
        "asset_name": file_path.stem,
        "file_path": str(file_path.relative_to(PROJECT_ROOT)).replace("\\", "/"),
        "game_path": game_path,
        "asset_type": asset_type,
        "file_size_bytes": file_size,
        "is_valid_uasset": is_valid_package,
        "inspection_mode": "fast_binary_scan"
    }

    if asset_type == "WidgetBlueprint":
        widgets, viewmodels, bindings = [], [], []
        for s in strings:
            if any(k in s for k in ["Button", "Text", "Canvas", "Image", "Progress", "Border", "List", "Overlay", "Box", "Slot"]):
                if len(s) < 40 and not s.startswith("/Script/") and not s.startswith("/Engine/") and s not in widgets:
                    widgets.append(s)
            if "ViewModel" in s and not s.startswith("/Script/") and s not in viewmodels:
                viewmodels.append(s)
            if any(s.startswith(prefix) for prefix in ["Get", "OnClicked", "OnHovered", "Bind_"]) and len(s) < 35:
                if s not in bindings:
                    bindings.append(s)
        metadata["detected_widgets"] = widgets[:25]
        metadata["detected_viewmodels"] = viewmodels[:10]
        metadata["detected_bindings"] = bindings[:15]

    elif asset_type in ["AnimMontage", "AnimSequence", "AnimBlueprint"]:
        notifies, slots, tracks = [], [], []
        for s in strings:
            if "Notify" in s and not s.startswith("/Script/") and s not in notifies and len(s) < 40:
                notifies.append(s)
            if any(k in s for k in ["Slot", "DefaultGroup", "UpperBody", "FullBody"]) and len(s) < 35 and s not in slots:
                slots.append(s)
            if "Track" in s and not s.startswith("/Script/") and s not in tracks and len(s) < 35:
                tracks.append(s)
        metadata["detected_notifies"] = notifies[:20]
        metadata["detected_slots"] = slots[:10]
        metadata["detected_tracks"] = tracks[:10]

    elif asset_type == "Blueprint":
        components, variables = [], []
        for s in strings:
            if "Component" in s and not s.startswith("/Script/") and not s.startswith("/Engine/") and len(s) < 40:
                if s not in components:
                    components.append(s)
            elif s.startswith("b_") or s.startswith("bIs") or s.startswith("Default_") or s.startswith("Target"):
                if len(s) < 35 and s not in variables:
                    variables.append(s)
        metadata["detected_components"] = components[:20]
        metadata["detected_variables"] = variables[:15]

    gameplay_tags = [s for s in strings if s.startswith("State.") or s.startswith("Ability.") or s.startswith("Event.") or s.startswith("Data.") or s.startswith("GameplayCue.")]
    if gameplay_tags:
        metadata["detected_gameplay_tags"] = sorted(list(set(gameplay_tags)))[:15]

    metadata["referenced_engine_classes"] = parent_classes[:12]
    return metadata

def parse_uasset_in_engine(game_path: str) -> dict:
    if not IN_ENGINE or not unreal:
        return {"error": "unreal python module not available"}
    asset = unreal.EditorAssetLibrary.load_asset(game_path)
    if not asset:
        return {"error": f"Failed to load asset in engine: {game_path}"}

    cls_name = asset.get_class().get_name()
    data = {
        "asset_name": asset.get_name(),
        "game_path": game_path,
        "engine_class": cls_name,
        "inspection_mode": "engine_reflection"
    }
    if cls_name == "WidgetBlueprint":
        tree = asset.get_editor_property("widget_tree")
        if tree:
            root = tree.get_editor_property("root_widget")
            data["root_widget"] = root.get_name() if root else "None"
            data["root_widget_class"] = root.get_class().get_name() if root else "None"
    elif cls_name == "AnimMontage":
        data["sequence_length"] = asset.get_editor_property("sequence_length") if hasattr(asset, "get_editor_property") else None
        notifies = []
        try:
            for n in asset.get_editor_property("notifies"):
                notifies.append({
                    "name": str(n.get_editor_property("notify_name")),
                    "trigger_time": round(float(n.get_editor_property("trigger_time_offset")), 3)
                })
        except Exception:
            pass
        data["notifies"] = notifies
    elif cls_name == "AnimBlueprint":
        target_skel = asset.get_editor_property("target_skeleton")
        data["target_skeleton"] = target_skel.get_path_name() if target_skel else None

    if hasattr(asset, "generated_class"):
        gen_cls = asset.generated_class()
        if gen_cls:
            data["generated_class"] = gen_cls.get_name()
            cdo = unreal.get_default_object(gen_cls)
            if cdo:
                data["cdo_class"] = cdo.get_class().get_name()
    return data

def locate_editor_cmd() -> Path | None:
    for env in ["UE_5_8_PATH", "UE_5_7_PATH", "UE_5_6_PATH", "UE_5_5_PATH", "UNREAL_ENGINE_PATH"]:
        v = os.environ.get(env)
        if v and os.path.exists(v):
            cmd = Path(v) / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
            if cmd.is_file(): return cmd
    candidates = [
        Path(r"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"),
        Path(r"C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"),
        Path(r"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"),
        Path(r"C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
    ]
    for c in candidates:
        if c.is_file(): return c
    return None

def run_deep_inspection_process(game_path: str) -> dict:
    editor_cmd = locate_editor_cmd()
    if not editor_cmd or not UPROJECT_FILE:
        return {"error": "UnrealEditor-Cmd or .uproject not found"}
    script_path = (PROJECT_ROOT / "Scripts" / "dump_asset_metadata.py").resolve()
    cmd = [
        str(editor_cmd), str(UPROJECT_FILE), "-run=pythonscript",
        f"-script={script_path} --asset {game_path}",
        "-NullRHI", "-NoSound", "-NoSplash", "-NoPause", "-SkipShaderCompile"
    ]
    try:
        proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
        for line in proc.stdout.splitlines():
            line_s = line.strip()
            if line_s.startswith("{") and line_s.endswith("}"):
                try: return json.loads(line_s)
                except Exception: pass
    except Exception as e:
        return {"error": str(e)}
    return {"error": "Failed to parse deep inspection JSON"}

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5 Asset Metadata Dumper")
    parser.add_argument("--asset", "-a", type=str, required=True, help="Asset path")
    parser.add_argument("--json", action="store_true", help="Emit raw JSON")
    parser.add_argument("--deep", action="store_true", help="Deep reflection inspection via UnrealEditor-Cmd")
    args = parser.parse_args()

    file_path, game_path = resolve_asset_paths(args.asset)
    if not file_path and not game_path:
        print(json.dumps({"status": "ERROR", "message": f"Asset not found: {args.asset}"}, indent=2))
        sys.exit(1)

    if IN_ENGINE and game_path:
        result = parse_uasset_in_engine(game_path)
    elif args.deep and game_path:
        result = run_deep_inspection_process(game_path)
        if "error" in result and file_path:
            result = parse_uasset_binary_fast(file_path, game_path)
    else:
        result = parse_uasset_binary_fast(file_path, game_path)

    print(json.dumps(result, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    main()
