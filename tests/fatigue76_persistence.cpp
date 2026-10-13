#include "TestHero.h"
#include "../firmware/RPG_POKET_2/FatigueJournal.h"
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
using namespace rpg;
struct PowerLoss {};
struct FaultStore {
  bool ready = true, tomb[3]{};
  std::map<std::string, std::vector<uint8_t>> blobs;
  unsigned operation = 0, cut = 0, returnedError = 0;
  bool after = false, lostAck = false, eraseNoop = false;
  void before() { ++operation; if (cut == operation && !after) throw PowerLoss{}; }
  void done() { if (cut == operation && after) throw PowerLoss{}; }
  void disarm() { cut = operation = returnedError = 0; }
  bool deleted(unsigned slot) { return tomb[slot]; }
  bool mark(unsigned slot, bool value) { before(); if (operation == returnedError && !lostAck) return false; tomb[slot] = value; done(); return operation != returnedError; }
  bool erase(const char* key) { before(); if (operation == returnedError && !lostAck) return false; if (!eraseNoop) blobs.erase(key); done(); return operation != returnedError; }
  Read readBytes(const char* key, uint8_t* out, unsigned size) {
    before(); if (operation == returnedError && !lostAck) return Read::Error; auto it = blobs.find(key); Read result;
    if (it == blobs.end()) result = Read::Missing;
    else if (it->second.size() != size) result = Read::Corrupt;
    else { memcpy(out, it->second.data(), size); result = Read::Ok; }
    done(); return operation == returnedError ? Read::Error : result;
  }
  bool writeBytes(const char* key, const uint8_t* bytes, unsigned size) {
    before(); if (operation == returnedError && !lostAck) return false; blobs[key] = {bytes, bytes + size}; done(); return operation != returnedError;
  }
  Read read(const char* key, uint8_t* out) { return readBytes(key, out, SAVE_SIZE); }
  bool write(const char* key, const uint8_t* bytes) { return writeBytes(key, bytes, SAVE_SIZE); }
  Read readFatigue(const char* key, uint8_t* out) { return readBytes(key, out, FATIGUE_RECORD_SIZE); }
  bool writeFatigue(const char* key, const uint8_t* bytes) { return writeBytes(key, bytes, FATIGUE_RECORD_SIZE); }
};
using Backend = FatigueSlotBackend<FaultStore>;
using Paired = FatigueJournal<Backend>;
std::vector<uint8_t> packed(const Game& game, uint32_t seq = 1) {
  std::vector<uint8_t> bytes(SAVE_SIZE); encode(game, seq, bytes.data()); return bytes;
}
bool same(const Game& a, const Game& b) { return packed(a) == packed(b) && a.fatigue == b.fatigue; }
Game hero(unsigned slot, unsigned fatigue) {
  auto g = create(slot, 501 + slot); g.tutorial = true; g.race = slot;
  g.shirt = slot + 1; g.trousers = slot + 3; g.fatigue = fatigue; return g;
}
unsigned boundaries = 0;
void verifyCuts(FaultStore original, unsigned slot, const Game& previous, const Game& next, bool empty) {
  Backend initial(original); initial.slot = slot; Paired journal(initial); Game loaded;
  assert(journal.load(loaded) == (empty ? Load::Empty : Load::Ok));
  original.disarm(); assert(journal.save(next)); unsigned count = original.operation;
  // Reconstruct the old starting state after deriving the number of operations.
  for (unsigned boundary = 1; boundary <= count; ++boundary) for (bool after : {false, true}) {
    FaultStore s;
    if (!empty) {
      Backend b(s); b.slot = slot; Paired seed(b); assert(seed.load(loaded) == Load::Empty);
      auto older = previous; older.fatigue = 3; older.p.gold = 2;
      assert(seed.save(older) && seed.save(previous));
    } else { s.tomb[slot] = true; s.blobs["fa"] = std::vector<uint8_t>(FATIGUE_RECORD_SIZE, 0x55); }
    Backend b(s); b.slot = slot; Paired j(b); assert(j.load(loaded) == (empty ? Load::Empty : Load::Ok));
    auto protectedBlobs = s.blobs;
    s.operation = 0; s.cut = boundary; s.after = after;
    try { (void)j.save(next); } catch (const PowerLoss&) {}
    s.disarm(); Paired reboot(b); auto status = reboot.load(loaded);
    if (empty) assert(status == Load::Empty || (status == Load::Ok && same(loaded, next)));
    else assert(status == Load::Ok && (same(loaded, previous) || same(loaded, next)));
    // Slots other than the active one are unchanged; an uncommitted staging
    // record may change only the inactive auxiliary, never the active pair.
    if (!empty && same(loaded, previous)) {
      assert(s.blobs[b.key(true)] == protectedBlobs[b.key(true)]);
      assert(s.blobs[b.fatigueKey(true)] == protectedBlobs[b.fatigueKey(true)]);
    }
    // Retry the original in-memory action, not exploration/ration/rest again.
    assert(reboot.save(next)); Paired retryBoot(b); assert(retryBoot.load(loaded) == Load::Ok && same(loaded, next));
    ++boundaries;
  }
}
int main() {
  static_assert(SAVE_SIZE == 128 && LEGACY_SAVE_FORMAT == 28, "Save28 contract");
  static_assert(FATIGUE_RECORD_SIZE == 140, "bounded auxiliary");
  FaultStore s; Backend b(s); Game loaded;
  for (unsigned slot = 0; slot < 3; ++slot) {
    b.slot = slot; Paired j(b); assert(j.load(loaded) == Load::Empty);
    auto g = hero(slot, 20 + slot); assert(j.save(g));
    auto raw = s.blobs[b.key(false)]; assert(raw == packed(g));
    Paired reboot(b); assert(reboot.load(loaded) == Load::Ok && same(loaded, g));
    auto next = g; next.p.gold += 17; --next.rations; next.fatigue += 4;
    assert(reboot.save(next));
    auto seeded = s; seeded.disarm();
    verifyCuts(seeded, slot, next, g, false);
  }
  // Per-character selection and corrupt auxiliary fallback do not alter either primary.
  for (unsigned slot = 0; slot < 3; ++slot) {
    b.slot = slot; Paired j(b); assert(j.load(loaded) == Load::Ok && loaded.fatigue == 24 + slot);
    auto clean = s.blobs; auto good = clean[b.fatigueKey(true)];
    for (unsigned offset = 0; offset < FATIGUE_RECORD_SIZE; ++offset) {
      s.blobs = clean; s.blobs[b.fatigueKey(true)][offset] ^= 0x5a;
      Paired corrupt(b); assert(corrupt.load(loaded) == Load::Ok && !loaded.fatigue && corrupt.fatigueReset);
      assert(s.blobs[b.key(false)] == clean[b.key(false)] && s.blobs[b.key(true)] == clean[b.key(true)]);
    }
    for (size_t length : {0u, 1u, 128u, 139u, 141u, 4096u}) {
      s.blobs = clean; s.blobs[b.fatigueKey(true)] = std::vector<uint8_t>(length, 0);
      Paired badLength(b); assert(badLength.load(loaded) == Load::Ok && !loaded.fatigue && badLength.fatigueReset);
    }
    s.blobs = clean; s.blobs.erase(b.fatigueKey(true)); Paired absent(b);
    assert(absent.load(loaded) == Load::Ok && !loaded.fatigue && absent.fatigueReset);
    s.blobs = clean;
    // Even a CRC-valid auxiliary from a different slot/side/snapshot is ignored.
    for (unsigned offset : {4u, 5u, 6u, 7u, 8u, 16u, 46u, 132u}) {
      auto foreign = good; foreign[offset] = offset == 7 ? 101 : foreign[offset] ^ 1;
      put32(foreign.data(), FATIGUE_RECORD_SIZE - 4, crc(foreign.data(), FATIGUE_RECORD_SIZE - 4));
      s.blobs[b.fatigueKey(true)] = foreign; Paired wrong(b);
      assert(wrong.load(loaded) == Load::Ok && !loaded.fatigue && wrong.fatigueReset);
    }
    s.blobs = clean;
  }
  // A valid older main copy recovers its paired fatigue. Both corrupt copies
  // remain blocked by the unchanged Journal; no auxiliary can resurrect a hero.
  b.slot = 0; auto clean = s.blobs;
  s.blobs[b.key(true)][20] ^= 1; Paired fallback(b);
  assert(fallback.load(loaded) == Load::Recovered && loaded.fatigue == 20);
  s.blobs[b.key(false)][20] ^= 1; auto damaged = s.blobs; Paired blocked(b);
  assert(blocked.load(loaded) == Load::Blocked && !blocked.save(hero(0, 4)) && s.blobs == damaged);
  s.blobs = clean;
  // Compound fault: stage into inactive auxiliary, lose active primary later.
  Paired staged(b); assert(staged.load(loaded) == Load::Ok); auto next = loaded; next.fatigue = 90;
  s.operation = 0; s.cut = 3; s.after = false;
  try { staged.save(next); } catch (const PowerLoss&) {}
  s.disarm(); s.blobs[b.key(true)][20] ^= 1; Paired compound(b);
  assert(compound.load(loaded) == Load::Recovered && !loaded.fatigue && compound.fatigueReset);
  s.blobs = clean;
  // Old firmware can read unchanged Save28. Once it writes another snapshot,
  // the stale auxiliary cannot modify the current character or resurrect fatigue.
  SlotBackend<FaultStore> legacy(b.store); legacy.slot = 0; Journal<SlotBackend<FaultStore>> old(legacy);
  assert(old.load(loaded) == Load::Ok && loaded.fatigue == 0); loaded.p.gold += 100;
  assert(old.save(loaded)); auto legacyBytes = s.blobs; Paired upgraded(b);
  assert(upgraded.load(loaded) == Load::Ok && !loaded.fatigue && upgraded.fatigueReset && s.blobs == legacyBytes);
  loaded.fatigue = 17; assert(upgraded.save(loaded)); Paired again(b); assert(again.load(loaded) == Load::Ok && loaded.fatigue == 17);
  // Explicit current-version replacement, including exactly the old hero bytes,
  // clears companion data before creating the new character; no fatigue transfer.
  assert(b.remove()); Paired empty(b); assert(empty.load(loaded) == Load::Empty);
  auto replacement = hero(0, 0); assert(empty.save(replacement)); Paired replaced(b);
  assert(replaced.load(loaded) == Load::Ok && same(loaded, replacement));
  FaultStore vacant; vacant.tomb[0] = true; vacant.blobs["fa"] = std::vector<uint8_t>(FATIGUE_RECORD_SIZE, 0x55);
  verifyCuts(vacant, 0, replacement, replacement, true);
  // A falsely acknowledged erase cannot expose a new hero with old auxiliary
  // data. Missing-record readback is required before any replacement staging.
  FaultStore stubborn; stubborn.tomb[0] = true; stubborn.blobs["fa"] = std::vector<uint8_t>(FATIGUE_RECORD_SIZE, 0x55);
  Backend sb(stubborn); Paired sj(sb); assert(sj.load(loaded) == Load::Empty);
  stubborn.eraseNoop = true; auto stale = stubborn.blobs;
  assert(!sj.save(replacement) && stubborn.tomb[0] && stubborn.blobs == stale);
  stubborn.eraseNoop = false; assert(sj.save(replacement)); Paired cleanReplacement(sb);
  assert(cleanReplacement.load(loaded) == Load::Ok && same(loaded, replacement));
  for (unsigned failure = 1; failure <= 4; ++failure) for (bool lostAck : {false, true}) {
    FaultStore cleanup; cleanup.tomb[0] = true; cleanup.blobs["fa"] = std::vector<uint8_t>(FATIGUE_RECORD_SIZE, 0x55);
    Backend cb(cleanup); Paired cj(cb); assert(cj.load(loaded) == Load::Empty);
    cleanup.operation = 0; cleanup.returnedError = failure; cleanup.lostAck = lostAck;
    assert(!cj.save(replacement) && cleanup.tomb[0] && !cleanup.blobs.count("a") && !cleanup.blobs.count("b"));
    cleanup.disarm(); assert(cj.save(replacement)); Paired verified(cb);
    assert(verified.load(loaded) == Load::Ok && same(loaded, replacement));
  }
  // Returned I/O errors keep the original live Journal. Even a committed
  // primary with a lost acknowledgement is retried at exactly the same seq.
  unsigned retries = 0;
  for (unsigned slot = 0; slot < 3; ++slot) for (unsigned failure = 1; failure <= 5; ++failure) for (bool lostAck : {false, true}) {
    FaultStore live; Backend lb(live); lb.slot = slot; Paired lj(lb);
    assert(lj.load(loaded) == Load::Empty); auto before = hero(slot, 38);
    assert(lj.save(before)); auto pending = before;
    --pending.rations; pending.p.gold += 6; pending.fatigue = 90;
    auto active = lj.active; auto seq = lj.seq;
    live.operation = 0; live.returnedError = failure; live.lostAck = lostAck;
    assert(!lj.save(pending) && lj.active == active && lj.seq == seq);
    live.disarm(); Paired observer(lb); assert(observer.load(loaded) == Load::Ok);
    assert(same(loaded, before) || same(loaded, pending));
    auto acknowledged = live.blobs; auto priorOperations = live.operation;
    assert(lj.save(pending) && lj.seq == seq + 1 && lj.active != active);
    if (failure == 5 || (lostAck && failure == 4)) assert(live.blobs == acknowledged && live.operation == priorOperations + 2);
    Paired boot(lb); assert(boot.load(loaded) == Load::Ok && same(loaded, pending));
    ++retries;
  }
  unsigned compensations = 0;
  for (bool fatigueOnly : {false, true}) for (unsigned boundary = 1; boundary <= 6; ++boundary) for (bool after : {false, true}) {
    FaultStore uncertain; Backend ub(uncertain); Paired uj(ub);
    assert(uj.load(loaded) == Load::Empty); auto before = hero(0, 67); before.p.gold = 100;
    assert(uj.save(before)); auto pending = before; if (fatigueOnly) pending.fatigue = 90; else pending.p.gold -= 25;
    uncertain.operation = 0; uncertain.returnedError = 5; uncertain.lostAck = true;
    assert(!uj.save(pending) && uj.seq == 1); // Primary committed; readback lost.
    uncertain.disarm(); auto committedMain = uncertain.blobs["b"], committedAux = uncertain.blobs["fb"];
    uncertain.cut = boundary; uncertain.after = after;
    try { uj.save(before); } catch (const PowerLoss&) {}
    uncertain.disarm(); Paired boot(ub); assert(boot.load(loaded) == Load::Ok);
    assert(same(loaded, pending) || same(loaded, before));
    assert(uncertain.blobs["b"] == committedMain && uncertain.blobs["fb"] == committedAux);
    assert(uj.save(before)); Paired compensated(ub);
    assert(compensated.load(loaded) == Load::Ok && same(loaded, before));
    ++compensations;
  }
  // A surprising newer inactive primary is protected rather than overwritten.
  FaultStore ahead; Backend ab(ahead); Paired aj(ab);
  assert(aj.load(loaded) == Load::Empty); auto baseline = hero(0, 20); assert(aj.save(baseline));
  auto newer = packed(baseline, 4); ahead.blobs["b"] = newer; auto futureState = ahead.blobs;
  assert(!aj.save(baseline) && ahead.blobs == futureState);
  // Returned failures through every recreation operation include a tombstone
  // clear whose acknowledgement was lost: retries recognize the committed pair.
  unsigned recreationRetries = 0;
  FaultStore countCreate; countCreate.tomb[0] = true;
  Backend countBackend(countCreate); Paired countJournal(countBackend);
  assert(countJournal.load(loaded) == Load::Empty); countCreate.disarm(); assert(countJournal.save(replacement));
  unsigned creationOperations = countCreate.operation;
  for (unsigned failure = 1; failure <= creationOperations; ++failure) for (bool lostAck : {false, true}) {
    FaultStore recreation; recreation.tomb[0] = true;
    Backend rb(recreation); Paired rj(rb); assert(rj.load(loaded) == Load::Empty);
    recreation.operation = 0; recreation.returnedError = failure; recreation.lostAck = lostAck;
    assert(!rj.save(replacement)); recreation.disarm();
    assert(rj.save(replacement)); Paired boot(rb);
    assert(boot.load(loaded) == Load::Ok && same(loaded, replacement)); ++recreationRetries;
  }
  // The same uncertain-commit reconciliation works across sequence rollover.
  for (bool compensate : {false, true}) {
    FaultStore rollover; Backend rb(rollover); Paired rj(rb);
    assert(rj.load(loaded) == Load::Empty); rj.seq = 0xfffffffeu;
    auto before = hero(0, 88); assert(rj.save(before)); auto pending = before; pending.fatigue = 91;
    rollover.disarm(); rollover.returnedError = 5; rollover.lostAck = true;
    assert(!rj.save(pending) && rj.seq == 0xffffffffu); rollover.disarm();
    const auto& wanted = compensate ? before : pending;
    assert(rj.save(wanted) && rj.seq == (compensate ? 1u : 0u));
    Paired boot(rb); assert(boot.load(loaded) == Load::Ok && same(loaded, wanted));
  }
  // Truncated writes to the inactive auxiliary or primary cannot destroy the
  // still-current pair. These simulate malformed storage as well as power cuts.
  for (unsigned target = 0; target < 2; ++target) for (unsigned length = 0; length < (target ? SAVE_SIZE : FATIGUE_RECORD_SIZE); ++length) {
    FaultStore partial; Backend pb(partial); Paired pj(pb);
    assert(pj.load(loaded) == Load::Empty); auto before = hero(0, 68); assert(pj.save(before));
    auto candidate = before; candidate.fatigue = 90; ++candidate.p.gold;
    uint8_t mainBytes[SAVE_SIZE], auxBytes[FATIGUE_RECORD_SIZE];
    encode(candidate, 2, mainBytes); encodeFatigue(mainBytes, 0, 1, candidate.fatigue, auxBytes);
    if (target) { partial.blobs["fb"] = {auxBytes, auxBytes + FATIGUE_RECORD_SIZE}; partial.blobs["b"] = {mainBytes, mainBytes + length}; }
    else partial.blobs["fb"] = {auxBytes, auxBytes + length};
    Paired boot(pb); auto status = boot.load(loaded);
    assert((status == Load::Ok || status == Load::Recovered) && same(loaded, before));
  }
  // Every explicit deletion boundary leaves either the old intact hero or an
  // empty tombstoned slot. No partially removed character can reappear.
  for (unsigned boundary = 1; boundary <= 5; ++boundary) for (bool after : {false, true}) {
    FaultStore deletion; Backend db(deletion); Paired dj(db);
    assert(dj.load(loaded) == Load::Empty); auto before = hero(0, 77); assert(dj.save(before));
    deletion.operation = 0; deletion.cut = boundary; deletion.after = after;
    try { db.remove(); } catch (const PowerLoss&) {}
    deletion.disarm(); Paired boot(db); auto status = boot.load(loaded);
    assert(status == Load::Empty || (status == Load::Ok && same(loaded, before)));
    if (status == Load::Empty) { auto fresh = hero(0, 0); assert(boot.save(fresh)); Paired rb(db); assert(rb.load(loaded) == Load::Ok && same(loaded, fresh)); }
    ++boundaries;
  }
  // Missing all companions and companion I/O faults remain visible fatigue-only
  // resets. Unsupported primary versions retain the original write protection.
  FaultStore missing; Backend mb(missing); Paired mj(mb);
  assert(mj.load(loaded) == Load::Empty); auto known = hero(0, 45); assert(mj.save(known));
  missing.blobs.erase("fa"); Paired noAux(mb);
  assert(noAux.load(loaded) == Load::Ok && !loaded.fatigue && noAux.fatigueReset);
  assert(noAux.save(known)); missing.operation = 0; missing.returnedError = 4;
  Paired ioFault(mb); assert(ioFault.load(loaded) == Load::Ok && !loaded.fatigue && ioFault.fatigueReset);
  missing.disarm(); auto future = missing.blobs["b"]; put16(future.data(), 4, CURRENT_SAVE_FORMAT + 1); missing.blobs["b"] = future;
  auto protectedFuture = missing.blobs; Paired futureReader(mb);
  assert(futureReader.load(loaded) == Load::Blocked && !futureReader.save(known) && missing.blobs == protectedFuture);
  // Unsigned sequence rollover retains the exact pair and existing ordering.
  FaultStore wrap; Backend wb(wrap); Paired wj(wb); assert(wj.load(loaded) == Load::Empty);
  wj.seq = 0xfffffffeu; auto w = hero(0, 98); assert(wj.save(w));
  w.fatigue = 99; ++w.p.gold; assert(wj.save(w) && wj.seq == 0);
  Paired rolled(wb); assert(rolled.load(loaded) == Load::Ok && same(loaded, w) && rolled.seq == 0);
  auto invalid = w; invalid.fatigue = 101; auto wrapBefore = wrap.blobs;
  assert(!rolled.save(invalid) && wrap.blobs == wrapBefore);
  printf("PASS: fatigue76 persistence %u before/after power-loss boundaries, 3 slots, 420 corrupt-byte cases, lengths/CRC identity, retry, replacement, legacy writes, primary recovery, %u live lost-ack retries, 268 truncated writes, %u compensation cuts, %u recreation lost-ack retries and sequence rollover\n", boundaries, retries, compensations, recreationRetries);
}
