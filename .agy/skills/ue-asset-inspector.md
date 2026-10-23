# Skill: ue-asset-inspector
Description: Ultra-fast binary inspection and reflection metadata extraction for Unreal Engine 5 BP, WBP, and Animation assets.

## When to Execute
Run this skill before writing or debugging C++ code that interacts with existing Blueprints (`BP_*`), Widget Blueprints (`WBP_*`), or Animations (`MM_*`, `ABP_*`).

## Actions
- **Inspect Editor Asset (`ue5_inspect_asset`)**:
  - Command: `python Scripts/dump_asset_metadata.py --json --asset <path>`
  - Inspects BP, WBP, Animation (AnimMontage, AnimBlueprint), and other `.uasset` files, outputting structured JSON metadata (widgets, notifies, viewmodels, parent classes, components).
- **Run Pipeline Inspection**:
  - Command: `python Scripts/pipeline.py inspect --asset <path>`
  - Formatted terminal report highlighting widget hierarchy, viewmodel contexts, anim notifies, and actor components.
