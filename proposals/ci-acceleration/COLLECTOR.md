# Benchmark collector: verification and timing boundary

`collect_benchmark.py` is an offline, benchmark-only collector. It produces a complete cold/warm pair and calls the existing comparator. It never downloads, extracts, compiles, runs artifact code, changes a workflow, or publishes firmware. The executable workflow supplies the downloaded inputs.

A successful result means that the supplied transport records, current Actions context, measured reports and checked payload bindings are consistent. It does **not** mean that a standalone JSON file authenticates itself, that a release is authorized, or that one observed pair establishes a general speedup.

## Invocation in the reviewed comparison job

```sh
python3 "$GITHUB_WORKSPACE/orchestrator/proposals/ci-acceleration/collect_benchmark.py" \
  --cold-id "$COLD_ARTIFACT_ID" \
  --cold-zip "$RUNNER_TEMP/comparison/cold.zip" \
  --cold-metadata "$RUNNER_TEMP/comparison/cold.metadata.json" \
  --warm-id "$WARM_ARTIFACT_ID" \
  --warm-zip "$RUNNER_TEMP/comparison/warm.zip" \
  --warm-metadata "$RUNNER_TEMP/comparison/warm.metadata.json" \
  --jobs "$RUNNER_TEMP/comparison/jobs.json" \
  --output "$RUNNER_TEMP/comparison/result.json"
```

All flags are required. Artifact IDs are canonical positive decimal strings, normalized explicitly to integers. The output is created only after both complete reports validate. An existing output path is never overwritten. A rejection exits unsuccessfully; no accepted report is generated.

The collector uses the actual inherited `GITHUB_*`, `RPG_DETECT_LEAKS=1` and `PYTHONDONTWRITEBYTECODE=1` environment via `benchmark_adapter.validate_context`. It does not fill in a synthetic Actions environment or offer an override flag. The two job contexts may differ only in their `cold`/`warm` phase. Source commit/tree/parent, orchestrator commit, exact benchmark ref, workflow identity, repository ID, run and attempt must agree with the existing adapter contract.

## Official transport inputs

The reviewed workflow uses its read-only Actions token to fetch:

- `GET /repos/{repository}/actions/artifacts/{artifact_id}`
- `GET /repos/{repository}/actions/artifacts/{artifact_id}/zip`
- `GET /repos/{repository}/actions/runs/{run_id}/attempts/{attempt}/jobs?per_page=100`, with `gh api --paginate --slurp`

The cold/warm IDs must come from the respective upload-step outputs. The collector does not search for a similarly named artifact or select the newest run. Metadata must identify the requested ID and exact name:

`benchmark-{source_sha}-{orchestrator_sha}-{run_id}-{attempt}-{phase}`

`workflow_run.id`, `repository_id`, `head_repository_id`, `head_sha` and `head_branch` must match. Fork provenance is rejected. The collector does not assume that artifact metadata has a `run_attempt` field: attempt binding comes from the exact artifact name, current context, attempt-scoped jobs response and creation time inside the selected job's start/completion interval. Artifacts must be unexpired with at least ten minutes of remaining retention.

Jobs input must be a nonempty array of complete API pages. Every page must agree on `total_count`, all job IDs must be unique, all returned jobs must match run/attempt/head/branch, and the collected count must equal the total. Intermediate pages contain 100 records. Exactly one job named `cold` and one named `warm` must have completed successfully. The warm job cannot start before the cold job completes. A running comparison job in the response is allowed.

The schema is based on GitHub's [artifact API](https://docs.github.com/en/rest/actions/artifacts#get-an-artifact) and [attempt-specific jobs API](https://docs.github.com/en/rest/actions/workflow-jobs#list-jobs-for-a-workflow-run-attempt). The actual downloaded JSON remains the caller's trust boundary: changing both a ZIP and its local metadata can forge local consistency, so output always retains `authenticated_provenance: false` and `publish_authorized: false`.

## ZIP and JSON handling

The complete ZIP's byte count and SHA-256 must exactly match the artifact API's `size_in_bytes` and `sha256:` digest before any member is consumed. The same open file descriptor is used for the subsequent checks.

This is the outer Actions transport ZIP, not the stricter inner release prototype. Both ordinary and ZIP64 streaming layouts, including data descriptors, are supported. No member is extracted. Checks reject traversal/absolute/ambiguous paths, NULs, duplicate names, file/directory collisions, overlapping members, mismatched local/central filenames, symlinks, executable regular files, special files, encryption and unsupported compression. Directory execute/search permission is allowed; directory payloads are not.

Limits are explicit:

- 8 GiB transport ZIP; 16 GiB aggregate expanded bytes
- 100,000 entries; 64 MiB central directory, bounded before Python's ZIP parser allocates it
- 512 MiB individual member; 32 MiB JSON; 256 MiB required aggregate/checkpoint log
- Streamed payload hashing in 1 MiB chunks, with CRC verification on consumed ZIP members
- Expansion ratio limited to 1000:1, with a 1 MiB allowance for small entries

JSON rejects duplicate keys and nonfinite numbers. The partial measurement and API inputs reject fractional values. Existing detailed identity/target/checkpoint evidence may contain finite floating-point observations, such as checkpoint seconds; these do not become measured nanosecond claims. Artifact scripts and captures are treated as bytes only.

## Evidence bindings

The root `measurement.partial.json` must have the exact schema emitted by `run_benchmark.py`: schema 1, kind `benchmark-partial`, phase, benchmark-only flags, adapter context, the complete comparator identity, cache, gates, coverage, seven internal phase durations and the release triple. Missing or extra fields are rejected.

The collector checks:

1. The source and reviewed-plan pins; SHA-256 of the local reviewed adapter; the current dependency lock and its PIN; exact run-scoped cache key and benchmark salt.
2. `benchmark-context.json`, successful `runner-status.json`, `phase-times.json` and `dependencies.json`. Dependency status/context/key/hit/lock/bytes/timing must bind to the partial. Cache bytes equal the locked download closure, and warm dependency download time is zero. Cache contents themselves are verified by the runner/helper; the collector does not receive or re-install the cache.
3. Recomputed streamed digests and sizes of `source/release/firmware.bin`, `artes.pak` and `manifest.json`, plus exact `SHA256SUMS` bytes.
4. `adapter/adapter-manifest.json`, all four generated adapter payload hashes, original script pins and a `benchmark_runtime.py` digest equal to the locally reviewed adapter. `commands.json` must record the five final release/OTA commands consecutively in the exact reviewed order, with exact arguments, the manifest's source working directory, integer exit status zero and positive monotonic timings. All five exact log paths, including OTA normal and sanitized execution, must exist and be nonempty. Logs are streamed with ZIP CRC checks; the official ZIP digest binds their contents without executing them.
5. `source/build/coopdiag77-benchmark-identity.json`: benchmark-only/cannot-publish/published-false flags, source/tree/context, adapter manifest hash, release hashes, LSan, target compiler/core/FQBN, both ON/OFF variants and default-ON firmware identity. The separate target-build evidence must equal the identity's target record. A production source-identity file is forbidden.
6. The exact 157-group summary, source-manifest recomputation and pinned inventory hash, reviewed commands and each checkpoint's passed status/command sequence/log digest. The aggregate log must contain exactly the 157 distinct `RUN` lines and no `RESUME skip` marker. Nested gate source hashes must match the pinned source inventory.
7. An exact staged source-artifact inventory and every listed payload's bytes/SHA-256, including capture files hashed as streams.
8. `compare_benchmark.validate` on both assembled reports and `compare_benchmark.compare` on the pair. These enforce identical source/scope/toolchain/image/adapter, independent jobs, exact cache provenance and all gates/coverage fields.

These are consistency checks against the recorded proof package. The collector does not rerun the 157 commands or infer a passed gate from an absent log. The reviewed runner is responsible for executing the full qualification from fresh workspaces and emitting a partial only after the gates succeed.

## Timing method

The runner records the seven internal phase totals using a monotonic nanosecond clock. The collector adds whole-second Actions API intervals, expressed in integer nanoseconds:

- Cold: `Cache save`; warm: `Cache restore`
- Both: `Upload benchmark evidence`
- Cold only: `Verify distributions before save`, added to `dependency_verify`

The helper's verification time and the runner's independent revalidation are already present in the partial. The separate pre-save workflow step is added once. That step may have a zero-second API interval; this means the work is below timestamp resolution, not that verification was free. Restore/save/upload must have positive successful API intervals. Missing, skipped or ambiguous overhead steps reject the comparison. Cold restoration and warm cache-save are zero only because those actions are absent.

`job_ns` is the selected job's API completion minus start. API timestamps are whole-second ISO-8601 observations; `Z` or explicit offsets with zero fractional digits normalize to UTC. Values are not claimed to have nanosecond accuracy. `timing_method` records the mixed measurement sources and their one-second API resolution. Job totals must cover the assembled phase totals. Negative measured savings are retained. Queue time, publication and generalized/fleet-wide savings are not inferred.

## Tests and scope

Run from a complete checkout containing the unchanged reviewed source files and source tree object:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover \
  -s proposals/ci-acceleration/tests -p test_collect_benchmark.py -v
PYTHONDONTWRITEBYTECODE=1 python3 -O -m unittest discover \
  -s proposals/ci-acceleration/tests -p test_collect_benchmark.py -v
```

Fixtures reconstruct the actual pinned source inventory from tree `5ad62cddfe052bea09bf99c29989731722088981`; production pins are not monkeypatched. Jobs, timings, release payloads and execution results are intentionally synthetic. Tests include the real CLI in normal and optimized Python, ZIP64/descriptors, digest/size mutation, wrong artifact/run/fork/attempt/head, incomplete/duplicate pagination, gate/cache/source drift, default-ON identity, checkpoint/command/log mutation, resumed groups, missing/zero/skipped overhead, final command omission/order/arguments/working-directory/exit-status drift, every missing or empty final log, final log mutation against the official ZIP digest, JSON ambiguity/nonfinite values and unsafe ZIP metadata.

Passing these tests establishes collector behavior only. It is not a new firmware qualification, measured cache saving, remote CI run or authorization to publish.
