# Rule: Autonomous Loop Engineering & Editor Asset Inspection Protocol

## 1. Autonomous Operating Loop
When assigned any gameplay implementation, bug fix, or refactoring task, autonomously execute:
1. **Investigate & Inspect**:
   - Trace relevant C++ classes in `Source/`.
   - Inspect any referenced `.uasset` files (Blueprints, Widget Blueprints, Montages) via `ue5_inspect_asset` or `python Scripts/pipeline.py inspect --asset <path>`.
   - Verify existing DataAsset, DataTable, or Blueprint property exposure before referencing assets; never assume or hardcode string paths.
   - Formulate plan grounded 100% in factual metadata.
2. **Execute**: Modify source code adhering strictly to dedicated server and card game architecture rules.
3. **Verify**: Run `ue-compile-verifier` (or `python Scripts/pipeline.py compile`). Auto-correct on failure (up to 5 retries).
4. **Test**: Run `ue-automation-tester` (or `python Scripts/pipeline.py test`) if game state logic was altered.
5. **Conclude**: Report changed files, inspection findings, and test results.

---

## 2. Editor Asset Inspection Protocol (에디터 에셋 인스펙션 프로토콜)

### 2.1. Principle: Ground Truth Over Guessing
- **Zero Hallucination Policy**: Never guess internal widget hierarchy names (`WBP_*`), animation notifies/slots (`MM_*`, `ABP_*`), or component structures (`BP_*`).
- Always extract verified metadata prior to writing C++ binding, RPC, or UI integration code.

### 2.2. Inspection Execution
- **Fast Static Scan (~0.005s, Default)**:
  ```bash
  python Scripts/pipeline.py inspect --asset <AssetPath_or_Name>
  ```
  or via MCP:
  ```json
  "ue5_inspect_asset" --asset <AssetPath_or_Name>
  ```
- **Deep Engine Reflection (`--deep`)**:
  When exact notify trigger timestamps or CDO runtime memory properties are required:
  ```bash
  python Scripts/pipeline.py inspect --asset <AssetPath_or_Name> --deep
  ```

### 2.3. Key Metadata Fields by Asset Type
- **WidgetBlueprint (`WBP_*`)**:
  - `detected_widgets`: RootCanvas, Button, TextBlock, ProgressBar, Image, etc.
  - `detected_viewmodels`: ModelViewViewModel (MVVM) contexts and instanced viewmodels.
  - `detected_bindings`: Event handlers (`Get*`, `OnClicked*`, `Bind_*`).
- **Animation (`MM_*`, `ABP_*`, `AnimSequence`)**:
  - `detected_notifies`: Montage and sequence notifies (`AN_*`).
  - `detected_slots`: Montage slot names (`DefaultGroup`, `UpperBody`, `FullBody`).
  - `detected_tracks`: Track definitions.
- **Actor Blueprint (`BP_*`)**:
  - `detected_components`: RootComponent, SceneComponent, PhaseComponent, etc.
  - `detected_variables`: Blueprint variables and default value references.
- **Gameplay Tags**:
  - `detected_gameplay_tags`: Tags prefixed with `State.*`, `Ability.*`, `Event.*`, `Data.*`, `GameplayCue.*`.
