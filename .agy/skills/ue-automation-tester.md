# Skill: ue-automation-tester
Description: Runs headless Unreal Engine Automation Tests via RunUAT for Dedicated Server card game gameplay validation.

## When to Execute
Run this skill when validating turn cycle state machines, deck draw logic, card resolution mechanics, and RPC validation handlers.

## Execution Procedure

### Step 1: Run Automation Tests
Execute via terminal:
```cmd
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\RunUAT.bat" RunTests -project="${workspaceFolder}\WildCard.uproject" -test="WildCard.CardGame" -nullrhi -nosound -unattended
```

### Step 2: Evaluate Results

* Parse the generated log for `Test Completed. Result={Passed|Failed}`.
* If tests fail, extract the assertion failures (`TestTrue`, `TestEqual`, `TestNotNull`).
* Formulate a fix targeting the gameplay logic and repeat the test until all assertions pass.
