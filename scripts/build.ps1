$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$frameworkRoot = Join-Path $projectRoot "tools\exlaunch"

if (-not (Test-Path -LiteralPath $frameworkRoot)) {
    throw "Clone exlaunch into tools\exlaunch before building."
}

$buildRoot = Join-Path $env:TEMP "hk-boss-hp-exlaunch"
New-Item -ItemType Directory -Path $buildRoot -Force | Out-Null
Copy-Item -Path (Join-Path $frameworkRoot "*") -Destination $buildRoot -Recurse -Force

Copy-Item -LiteralPath (Join-Path $projectRoot "src\main.cpp") -Destination (Join-Path $buildRoot "source\program\main.cpp") -Force
Copy-Item -LiteralPath (Join-Path $projectRoot "exlaunch\config.mk") -Destination (Join-Path $buildRoot "config.mk") -Force

$loaderPatch = Join-Path $projectRoot "exlaunch\patches\0001-unity-main-module-fallback.patch"
& git -C $buildRoot apply --whitespace=nowarn $loaderPatch
if ($LASTEXITCODE -ne 0) {
    throw "Could not apply the Hollow Knight exlaunch compatibility patch."
}

$commonMakefile = Join-Path $buildRoot "misc\mk\common.mk"
$commonMakefileText = Get-Content -LiteralPath $commonMakefile -Raw
$linkedMakefileText = $commonMakefileText -replace '(?m)^LIBS\s*:=\s*$', 'LIBS := -lnx'
$linkedMakefileText = $linkedMakefileText -replace '(?m)^LIBDIRS\s*:=\s*$', 'LIBDIRS := $(LIBNX)'
if ($linkedMakefileText -eq $commonMakefileText) {
    throw "Could not enable libnx linking for telemetry."
}
Set-Content -LiteralPath $commonMakefile -Value $linkedMakefileText -NoNewline

$msysBash = "C:\devkitPro\msys2\usr\bin\bash.exe"
if (-not (Test-Path -LiteralPath $msysBash)) {
    throw "devkitPro MSYS2 was not found at $msysBash."
}

$msysProject = $buildRoot.Replace("\", "/").Replace("C:", "/c")
& $msysBash -lc "cd '$msysProject' && sed -i 's/\r$//' config.mk && make clean && make -j4"
if ($LASTEXITCODE -ne 0) {
    throw "exlaunch build failed with exit code $LASTEXITCODE."
}

$dist = Join-Path $projectRoot "dist\atmosphere\contents\0100633007D48000\exefs"
New-Item -ItemType Directory -Path $dist -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $buildRoot "deploy\subsdk9") -Destination (Join-Path $dist "subsdk9") -Force

$hactool = Join-Path $projectRoot "tools\hactool\hactool.exe"
$npdmtool = "C:\devkitPro\tools\bin\npdmtool.exe"
$originalNpdm = Join-Path $projectRoot "dump\update\exefs\main.npdm"
$generatedJson = Join-Path $projectRoot "exlaunch\main-generated.json"

if (-not (Test-Path -LiteralPath $hactool)) {
    throw "hactool was not found at $hactool."
}
if (-not (Test-Path -LiteralPath $originalNpdm)) {
    throw "The original Hollow Knight main.npdm was not found."
}

& $hactool -t npdm "--json=$generatedJson" $originalNpdm | Out-Null
if ($LASTEXITCODE -ne 0) {
    throw "Could not decode the original main.npdm."
}

$npdm = Get-Content -LiteralPath $generatedJson -Raw | ConvertFrom-Json
$syscalls = $npdm.kernel_capabilities |
    Where-Object { $_.type -eq "syscalls" } |
    Select-Object -First 1

$requiredSyscalls = [ordered]@{
    svcCreateCodeMemory = "0x4b"
    svcControlCodeMemory = "0x4c"
    svcInvalidateProcessDataCache = "0x5d"
    svcStoreProcessDataCache = "0x5e"
    svcSetProcessMemoryPermission = "0x73"
    svcMapProcessMemory = "0x74"
    svcUnmapProcessMemory = "0x75"
    svcQueryProcessMemory = "0x76"
    svcMapProcessCodeMemory = "0x77"
    svcUnmapProcessCodeMemory = "0x78"
}

foreach ($entry in $requiredSyscalls.GetEnumerator()) {
    $syscalls.value | Add-Member -NotePropertyName $entry.Key -NotePropertyValue $entry.Value -Force
}

$debugFlags = $npdm.kernel_capabilities |
    Where-Object { $_.type -eq "debug_flags" } |
    Select-Object -First 1
$debugFlags.value.allow_debug = $false
$debugFlags.value.force_debug = $false
$debugFlags.value | Add-Member -NotePropertyName force_debug_prod -NotePropertyValue $true -Force

$npdm | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath $generatedJson -Encoding UTF8
& $npdmtool $generatedJson (Join-Path $dist "main.npdm")
if ($LASTEXITCODE -ne 0) {
    throw "Could not generate the patched main.npdm."
}

Write-Host "Built: $dist\subsdk9"
Write-Host "Built: $dist\main.npdm"
