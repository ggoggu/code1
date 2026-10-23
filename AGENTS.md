# WildCard Autonomous Agent Harness & Loop Configuration

## 1. Operating Loop Directive & Pipeline Modes
Supported pipeline modes (`Scripts/pipeline.py`):
- `inspect`: Ultra-fast uasset binary & deep reflection inspection (`--asset <path>` [--deep] [--json])
- `compile`: Autonomous UBT compilation for Dedicated Server and Editor targets
- `test`: Headless RunUAT card game gameplay validation
- `all`: Full compile and test verification

When assigned any gameplay implementation, bug fix, or refactoring task, you MUST autonomously execute the following loop without asking for step-by-step confirmation:

1. **Investigate & Inspect**:
   - Trace relevant C++ classes in `Source/`, identify existing patterns, and verify replication setup.
   - For any interacting Blueprints (`BP_*`), Widget Blueprints (`WBP_*`), or Animations (`MM_*`, `ABP_*`), execute `python Scripts/pipeline.py inspect --asset <path>` (or MCP `ue5_inspect_asset`) to extract ground-truth metadata before designing changes.
2. **Execute**: Maximize the reuse of existing classes, functions, and systems before creating new ones. Modify the source code strictly adhering to `.agy/rules/ue5-dedicated-server.md`, `.agy/rules/card-game-architecture.md`, and `.agy/rules/03-loop-engineering.md`.
3. **Verify (Skill Loop)**:
   - Call `ue-compile-verifier` (or `python Scripts/pipeline.py compile`) for `WildCardServer` and `WildCardEditor`.
   - If compile fails, parse errors, self-correct the code, and re-run compilation (up to 5 autonomous retry iterations).
4. **Test**: Call `ue-automation-tester` (or `python Scripts/pipeline.py test`) if game state logic was altered.
5. **Conclude**: Report changed files and final compilation/test results to the user.

## 2. Guardrails & 최소 수정 및 구조 보존 원칙
- **Ground Truth Over Guessing (에셋 추측 금지 & 메타데이터 확인)**:
  기존 블루프린트(`BP_*`), 위젯(`WBP_*`), 애니메이션(`MM_*`, `ABP_*`)과 상호작용하는 코드를 작성하거나 디버깅할 때, 내부 위젯명/슬롯명/노티파이를 임의로 추측(Hallucination)하지 마라. 반드시 작업 전 `python Scripts/pipeline.py inspect --asset <path>` (또는 MCP `ue5_inspect_asset`)를 실행하여 덤프된 JSON 메타데이터를 확인하고 코드를 작성해라.
- Always maximize the reuse and extension of existing code and architecture; NEVER duplicate existing functionality or create redundant abstractions.
- **No Hardcoded Asset/Network Paths (경로 및 주소 하드코딩 금지)**:
  C++ 소스 내부에 `/Game/...` 형태의 uasset 문자열 경로, IP/Port(`127.0.0.1:7777`), 레벨명 리터럴을 직접 박아넣지 마라. 에셋 참조는 `TSubclassOf<T>`/`TSoftObjectPtr<T>`를 `EditDefaultsOnly`로 노출하고, 접속 주소/레벨명은 Config(ini) 또는 GameInstance/Subsystem을 통해 주입받아라.
- **Separation of Controller and UI (컨트롤러-UI 인라인 결합 금지)**:
  PlayerController 소스 파일 내부에 수백 줄의 Slate 위젯(`SCompoundWidget` 등)을 인라인으로 정의하지 마라. 컨트롤러는 입력 수신 및 네트워크 RPC 중계에 집중하고, UI 구현체는 반드시 별도 UI 파일로 분리하라.
- NEVER modify engine source code under `UE_5.7/Engine/`.
- NEVER disable compiler warnings or remove network validation checks (`_Validate`) to bypass errors.
- Always keep `#include "FileName.generated.h"` as the last include in headers.
