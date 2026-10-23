---
name: ue-asset-inspector
description: >-
  Inspects Unreal Engine 5 BP, WBP, Animation (MM/ABP), and other binary assets,
  extracting structured metadata JSON (widgets, viewmodels, notifies, slots, components)
  in ~0.005s to ground AI pair programming decisions in factual truth.
---

# UE Asset Inspector Skill

## Actions
- **Inspect Editor Asset (`ue5_inspect_asset`)**:
  - Command: `python Scripts/dump_asset_metadata.py --json --asset <path>`
  - Inspects BP, WBP, Animation (AnimMontage, AnimBlueprint), and other `.uasset` files, outputting structured JSON metadata (widgets, notifies, viewmodels, parent classes, components).
- **Run Pipeline Inspection**:
  - Command: `python Scripts/pipeline.py inspect --asset <path>`
  - Formatted terminal report highlighting widget hierarchy, viewmodel contexts, anim notifies, and actor components.

## When to Execute
- Before implementing or editing C++ code that references Widget Blueprint components or animations.
- When verifying component hierarchies in Actor Blueprints (`BP_*`).
- When checking animation slot names (`DefaultGroup`, `UpperBody`) or notify events (`AN_*`).
- When binding MVVM ViewModel properties to Widget Blueprints (`WBP_*`).
