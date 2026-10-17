# Wifitest77: qualification contract

Candidate: `2026.10.17-wifitest77`, build `2026101701`, package `7.7 - Teste Wi-Fi`.
Qualification branch: `feat/wifitest77-release`; future tag: `v2026.10.17-wifitest77`.
Sole parent and main lease: `4fc221f07b62d6bd26fcf03fa1bb12c8fd577111`, tree `89c4252b249e144ced3382fd85e3476e122d3433`.

The source inventory is deliberately fail-closed until independent review of the final reconstructed source. The missing earlier candidate is not a source or binary baseline. No prior incremental test result qualifies these bytes. Promotion must stop on source, ancestry, main-lease, tag, workflow or artifact drift.

## Scope and limits

This is an isolated, opt-in Wi-Fi radio test opened from Settings. It has its own fixed wire identity and state, distinct from cooperative combat and PvP. Its controls select initiator or responder, discovery, explicit session consent, cancellable probing, local readbacks and requested TX-power presets. The adapter exclusively owns the radio during the test, uses bounded callback storage, fences stale generations and restores the original driver profile on exit. Incomplete restoration retains ownership and exposes a retryable error rather than silently resuming another radio user.

Requested power and successful setters are not measurements of radiated power. The firmware separately reports requested and read-back values; country limits and the sampled baseline bound available settings. USB PHY and reduced-power configuration flags describe compiled support only. MAC-send completion is not an application ACK. Discovery at short distance, a host fake or a software pass cannot establish the RF cause or prove a range repair. Two-device RF/touch/OTA, runtime stack/heap and frame-time measurements remain separate physical work.

The RAM result ring is bounded to 24 entries. CSV export is an explicit user action to SD and records diagnostic results; it does not rewrite Wi-Fi credentials, save journals or the authored art pack. Export failure must preserve the ring and existing files. Automatic diagnostic persistence or hidden background scans are not part of this feature. Exact controls, protocol phases, counter semantics and CSV columns are checked against the reconstructed implementation, not against the missing candidate.

Save30 remains 160 bytes, with readers 29/30, reset epoch 1, three slots and ration limit 10. There is no new reset. Campaign RNG, SHOP, PvP5/52-byte packets, existing page indices, updater partitioning and all 666 authored assets remain frozen. The current main's README, cooperative control/hardware documents, current state, quick guide and index are preserved byte-for-byte. Historical gate/test files are unchanged; only the two reviewed title/dungeon UI snapshots are permitted changes to inherited tests.

## Exact source and complete host qualification

`tools/check_wifitest77_scope.py` binds the reviewed parent/tree, complete path/hash/mode inventory and independently approved production hashes. It rejects missing approval, unknown paths, source drift, symlink escapes, changed historical files and generated firmware tracked as source. There is no auto-accept or inventory-refresh command. Every new Python gate refuses optimized Python, including `-O`, `-OO` and `PYTHONOPTIMIZE=1`.

The official zero-argument entrypoint is:

```sh
python3 -m pip install 'Pillow==11.3.0' 'PyYAML==6.0.3'
export RPG_DETECT_LEAKS=1 PYTHONDONTWRITEBYTECODE=1
bash tools/test_wifitest77.sh
```

The runner imports all 156 historical task groups unchanged and appends eight groups: model, flow, device, callback races, CSV export, actual runtime controls, render-on and render-off. Together with preparation there are 165 groups. Each new suite runs normally and with ASan/UBSan/LSan 1. New render evidence requires 22 named RADIO1 frames and one RADIO0 internet-test frame at 240×320, with exact normal/sanitized pixel parity; screenshots remain host evidence. The unchanged checkpoint scheduler runs integration groups after the inherited pool; source/compiler/environment, exact plan, checkpoint and log hashes remain required. A focused run, cached result from different inputs, disabled leak detection or direct local runner invocation does not qualify a release. LSan inability is a blocked or failed qualification, never permission to disable it.

The complete inherited installer/recovery gates remain mandatory, including:

```sh
python3 tests/coop_update_install_equivalence.py
python3 tools/test_coop_update_install.py --mode both --output-dir build/coop-training-native
python3 tools/test_coop_radio_recovery_regression.py --mode both --output-dir build/coop-radio-recovery
```

The six installer cases each run in two modes: linkage, adapter, recovery, art readback, SD diagnostics and enemy OTA. The real `UpdateInstall.cpp` is compiled as a separate translation unit with real SHA256; singleton/dependency boundaries and C++ assertions remain enforced. Historical captures, recovery behavior, invite controls and game/persistence assertions are retained, independently of new radio evidence.

## Target builds and byte gates

Use core `esp32:esp32@3.3.12`, compiler `esp-x32/2601`, and exactly:

`esp32:esp32:esp32s3:CDCOnBoot=cdc,USBMode=hwcdc,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,EraseFlash=none`

Three separately logged external builds are required:

1. COOP1/RADIO1, using Arduino defaults with no ON override; this is the only published image.
2. COOP1/RADIO0, only `-DRPG_ENABLE_WIFI_RADIO_TEST=0`.
3. COOP0/RADIO1, only `-DRPG_ENABLE_COOP_TRAINING=0`.

All builds use `--verbose --jobs 4` and `--export-binaries=false`. Compiler/linker workarounds, outputs inside the sketch and alternate partitions are rejected. Evidence binds effective properties/compile commands, exact image identity/envelope, memory budgets and singleton/ODR/art/updater boundaries for every variant. The 3 MiB OTA limit and known PSRAM total 7638272 bytes are unchanged. Static budgets are not measured free heap. New radio state is recorded separately from the inherited co-op diagnostic state: one Device at most 8192 bytes, one mailbox at most 2048 bytes, one UI snapshot at most 2048 bytes and one 4-byte generation counter. Total per-feature image and static-RAM increments retain the inherited caps of 262144 and 16384 bytes.

The actual compiled candidate triple must pass manifest generation, independent verifier mutations and normal/sanitized OTA tests. Synthetic fixtures explicitly test adapters only and cannot substitute for a compiled image or target evidence. Thirteen immutable historical clients are exercised: touch77 refuses; scenes77, scale77, camp77, explore77, immersion77, story77, clock77, pvp77, shop77, coop77, coopfix77 and coopdiag77 accept according to their actual contracts. All quoted-header closures and historical split installers are byte-exact. Save29→30 lazy migration and Save30→30 retention cover all slots, geometry/RNG, epoch, Wi-Fi and art; loads and rollback probes do not write. Pre-shop readers may reject ration 10; firmware downgrade is not promised.

The added coopdiag77 public baseline is source `6026fb49db4fe874b9c3f0571cbe4fdb9e66adb3`, tree `5ad62cddfe052bea09bf99c29989731722088981`. Fresh official downloads were independently verified on 2026-10-17: firmware 2964656 bytes/SHA256`600fd558cc3d8b2852470f7a48b23250c23a4370f456197a54a8878a673000a6`; art 7470928/SHA256`14bb1adc47bb3e4bdde510f80d165bc8622129f0422e2aef908e97b079028c56`; manifest 1228/SHA256`baa489750d50553c22027a6b5f6853364838f0c927c7144aff43bbfd567744e0`. The exact documentation-only transition from that release to main 4fc221f is pinned separately.

## Qualification, publication and readback

Qualification is push-only on the exact branch/repository with `contents: read`. The publisher is push-only on main with `contents: write` and `actions: read`. Neither accepts manual dispatch. The selected publisher requires completed successful qualification of the exact source SHA, repository, branch and workflow path, then repeats full host/LSan qualification, all three target builds and actual-byte gates. It does not promote downloaded qualification artifacts or activate the separate artifact-promotion experiment.

The tag must be unused before rebuilding and immediately before creation. A fresh live-main read must still equal the checked-out SHA immediately before release creation; this preflight is not an atomic lock. Independent source/artifact review and a fresh parent lease precede promotion of the exact qualified commit. Rebase, squash or any other new commit requires fresh qualification.

Only `firmware.bin`, `artes.pak` and `manifest.json` are public release assets. Evidence artifacts contain bounded reports/logs/captures/headers and the candidate triple, not host executables, target ELF/objects or fallback firmware. After publishing, all three public downloads must byte-match the publisher outputs; the downloaded triple reruns the verifier and normal/sanitized real-byte OTA gates. Latest-manifest bytes must also match. Publication does not establish physical-board validation.
