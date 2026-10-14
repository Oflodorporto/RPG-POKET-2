# Package 6 local integration candidate

This is an unpublished native-test staging tree. It is not qualified firmware,
and the inherited `camp77` identity must not be used to build or publish it.
No firmware build, device installation, remote mutation, reset, tag or release
has been performed by this integration task. The local checkout has no remote.

## Inputs and preserved scope

- Frozen package5 source tree: `b58a5c80339f1d87728723991efdc8006ff764f8`.
- Combat patch SHA-256:
  `609b0aa17168de59cd4100cfaa0b966080cdd52aef5497c7e3121f686b28dfd6`.
- Procedural patch SHA-256:
  `086bb148b2ce9c1b95f1490f19df205ea52aaecba9adab6307b9da980feb3707`.
- Frozen R5 replacement applied over that original procedural patch:
  `8c1002c4955d00d0eedaeec3c6dc3af2aa6a67a4ed5a952662164eac4a2afef6`.
  The equivalent full procedural diff against `ad8acd8` has SHA-256
  `c4bb4bb86deb4225acf99fa2c7682ab322b0d0c568cc80b2bc5dd4180b1eb4bc`.
- Both patches apply without textual conflicts. Package5's D20, common camp,
  personalized hero, tent marker, reusable art and Ruins bag remain integrated.
- The actual controller still matches both controller snapshots verbatim.
- Release identity, package5 qualification sources/manifest and workflows are
  unchanged. The unchanged package5 scope gate correctly rejects this tree.
- Only new paid Ruins runs use generated geometry. Island and letter-expedition
  maps remain fixed. Combat math, real RNG, enemy/reward identities and entrance
  cost are preserved. Trace capture does not fabricate or repeat a die roll.

## Integration-specific changes

1. Marked the checkout and README as local staging, with explicit publication
   restrictions and unresolved procedural gameplay-variety review.
2. Minified the new procedural readback JSON fixture. The original readback test
   deliberately edits exact compact JSON tokens; the pretty-printed input could
   not exercise that path. The JSON values and runtime parser are unchanged.
3. Extended the combat differential test with an explicit Save30 mode, keeping
   its default strict Save29 mode. Save30 requires the separately frozen
   procedural-only baseline, pinned by SHA-256 for all 12 dependency headers.
   This compares baseline/default/null/captured resolvers under the same new
   Game/save ABI, including retained and active procedural-run metadata.
4. Added a combined actual-controller test for package5 bag navigation, paid
   generated entry, real combat receipt views, timer/input quarantine and reload
   across three slots, four classes and eight seeds.
5. Added a native-only reproduction runner, `tools/test_package6_local.sh`.
   Historical fixed-geometry fixtures explicitly request `enterDungeon(g,0)`;
   production entry and the generated-map tests keep the procedural default.
   The historical Save28 controller restores its pre-entry fixture before that
   explicit call, and the fixed-coordinate trace screenshot also requests the
   fixed map. R5 generated-map coverage is preserved unchanged.

No production merge correction was required beyond the two input patches and
the frozen R5 replacement. R5's early graph/guard variation and open arrival
view preserve the original pickup fight prerequisites; Save30 layout is unchanged.
An initial aggregate setup failure was a missing retained historical Git object;
it was recovered from the frozen source checkout without changing any test gate.

## Verification status

The final test summary and exact source/output hashes accompany this candidate.
Individual results are not permission to publish. The complete original native
aggregate, feature suites, sanitizer suites and combined tests must all finish
successfully before technical integration is declared complete.

The independent Save29 review was copied to an isolated review directory and
rerun against the integrated 15-header closure. It passed all 9,216 canonical
released Save29 fixtures (9,124 active fixed-map runs), 75 rollback probes,
18 released NVS rollback probes, 210+210 journal power-cut/fault probes and
90+225 production NVS power-cut/fault probes. Existing Save29 runs, progression,
RNG, other slots, reset epoch, Wi-Fi and art remain preserved.

Trace receipts are RAM-only. The full Game now has 184 bytes on the host ABI and
Save30 remains 160 bytes. Save30 slots cannot be played by old Save29 firmware:
the old reader fails closed without erasing them. Compatible firmware is needed
to reopen those slots. There is no new reset or bulk migration.

## Remaining decisions and limits

- Procedural visual/gameplay variety remains under product review. Distinct
  topology hashes and validity do not establish satisfactory gameplay uncertainty.
- A new version/build and Save30-aware release/manifest gate must be selected
  separately. The inherited package5 gates are intentionally not rewritten.
- ESP32 compile/size/partition margin and physical-device touch, memory, frame
  timing and brownout validation are not established by host tests.
- ASan and UBSan run with leak detection disabled because LeakSanitizer cannot
  operate under this executor's ptrace environment. No leak-sanitizer pass is
  claimed.
