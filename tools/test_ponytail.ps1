param([Parameter(Mandatory=$true)][string]$Zig, [string]$Out = 'build/ponytail-host', [string[]]$Only = @())
$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$outPath = [IO.Path]::GetFullPath($Out)
New-Item -ItemType Directory -Path $outPath -Force | Out-Null
$env:ZIG_LOCAL_CACHE_DIR = Join-Path (Split-Path $outPath -Parent) 'zig-cache'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path (Split-Path $outPath -Parent) 'zig-global'
$fixture = [IO.File]::ReadAllText((Join-Path (Get-Location) 'tests/package12_coop_controller.cpp')).Replace('int main(){','void frozen_package12_controller_main(){')
[IO.File]::WriteAllText((Join-Path $outPath 'coop_controller_fixture.generated.h'),$fixture)
$invite = [IO.File]::ReadAllText((Join-Path (Get-Location) 'tests/package12_coop_controller.cpp')).Replace('int main(){','void frozen_package12_controller_main(){')
[IO.File]::WriteAllText((Join-Path $outPath 'coop_invite_controller_fixture.generated.h'),$invite)
$results = @()
$blocked = @(
    @{suite='combat_trace_differential';reason='Requires GCC __builtin_clear_padding and the historical runner selecting Save30; Zig/Clang cannot execute this gate.'},
    @{suite='dungeon77_upgrade';reason='Requires a separately frozen recovery77 header tree plus historical release bytes. Not a test against current headers.'}
)
$blocked | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outPath 'not-run.json') -Encoding utf8
$core = @('save29','reset29','persistence29_adapter','daily_travel29','daily_travel29_combat','escort_recovery29','rules_save','gear_save','forge_save','dungeon_save','dungeon_controller','class_progression','class_powers','martial_powers','balance74','long_session','bulk_commerce','touch77_controller','campaign_finale','guild_save','update_adapter','boot_art_diagnostics','shared_spi_startup','ota_sd_diagnostics','windows_compat')
$radio = @('coop_training_session','coop_training_radio','coop_training_input','coop_training_device','coop_cleanup','coop_radio_recovery_regression','coop_radio_runtime_ui','coop_touch_queue_regression','coop_invite_countdown_regression')
$wifi = @('wifi_radio_test_model','wifi_radio_test_flow','wifi_radio_device','wifi_radio_device_races','wifi_radio_test_export','wifi_radio_test_runtime')
$pvp = @('club_protocol','club_duel','fatigue76_club_radio','pvp_reliability_protocol','pvp_reliability_radio','pvp_reliability_races','pvp_reliability_closing','pvp_reliability_persistence','pvp_reliability_ordering','pvp_reliability_save30','pvp_review_integration','pvp_radio_cleanup')
foreach ($name in ($core + $radio + $pvp + $wifi)) {
    if ($Only.Count -and $name -notin $Only) { continue }
    $extra = @("-I$outPath",'-Itests','-Ifirmware/RPG_POKET_2')
    if ($radio -contains $name) { $extra += '-Itests/coop_fake' }
    if ($name -in @('coop_training_input','coop_training_device','coop_cleanup','coop_radio_runtime_ui','coop_touch_queue_regression','coop_invite_countdown_regression','coop_radio_recovery_regression')) { $extra += @('-DRPG_ENABLE_COOP_TRAINING=1','firmware/RPG_POKET_2/CoopRuntime.cpp') }
    if ($pvp -contains $name) { $extra += @('-Itests/club_fake','-Ifirmware/RPG_POKET_2','firmware/RPG_POKET_2/ClubProtocol.cpp','firmware/RPG_POKET_2/ClubDuel.cpp') }
    if ($name -eq 'pvp_radio_cleanup') { $extra += '-DRPG_ENABLE_COOP_TRAINING=1' }
    if ($wifi -contains $name) { $extra += @('-Itests/wifi_radio_fake','-DRPG_ENABLE_WIFI_RADIO_TEST=1') }
    if ($name -in @('update_adapter','ota_sd_diagnostics')) { $extra += @('-x','c++','-Itests/update_fake','tests/update_fake/cJSON.c','firmware/RPG_POKET_2/UpdateInstall.cpp') }
    if ($name -eq 'boot_art_diagnostics') { $extra += @('-Itests/settings_fake','-include','tests/windows_compat.h') }
    if ($name -eq 'shared_spi_startup') { $extra += '-Itests/shared_spi_fake' }
    $exe = Join-Path $outPath ($name + '.exe')
    # Windows PowerShell 5 promotes compiler warnings on stderr to errors when
    # ErrorActionPreference is Stop. Keep the real exit code authoritative.
    $ErrorActionPreference = 'Continue'
    & $Zig c++ -std=c++17 -O2 -UNDEBUG "tests/$name.cpp" @extra -o $exe *> (Join-Path $outPath ($name + '-compile.log'))
    $compiled = $LASTEXITCODE -eq 0; $passed = $false
    if ($compiled) { & $exe *> (Join-Path $outPath ($name + '.log')); $passed = $LASTEXITCODE -eq 0 }
    $ErrorActionPreference = 'Stop'
    $results += [pscustomobject]@{suite=$name;compiled=$compiled;passed=$passed}
    $results | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $outPath 'results.json') -Encoding utf8
    Write-Output "$name compiled=$compiled passed=$passed"
}
if ($results.Where({ -not $_.passed }).Count) { throw 'Native regression failed; inspect results.json and suite logs' }
Write-Output "PASS: $($results.Count) selected native suites; two unavailable historical gates listed in not-run.json. This is not the Linux 165-group release qualification or sanitizer/RF evidence."
