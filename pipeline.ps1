param(
    [Parameter(Position=0)]
    [string]$Mode = "inspect",
    [Parameter(Position=1)]
    [string]$Asset,
    [switch]$Deep,
    [switch]$Json,
    [string]$Target = "both"
)

$argsList = @("Scripts/pipeline.py", $Mode)

if ($Asset) {
    $argsList += @("--asset", $Asset)
}
if ($Deep) {
    $argsList += "--deep"
}
if ($Json) {
    $argsList += "--json"
}
if ($Mode -in @("compile", "build")) {
    $argsList += @("--target", $Target)
}

python @argsList
exit $LASTEXITCODE
