#pragma once
#include "Slots.h"

// Save28 stays the authority. This companion contains an exact copy of the
// committed 128-byte save (including sequence and CRC), never a new save format.
namespace rpg {
constexpr unsigned FATIGUE_RECORD_SIZE = SAVE_SIZE + 12;
inline void encodeFatigue(const uint8_t* save, uint8_t slot, uint8_t side,
                          uint8_t fatigue, uint8_t* out) {
  memcpy(out, "FAT6", 4);
  out[4] = 1; out[5] = slot; out[6] = side; out[7] = fatigue;
  memcpy(out + 8, save, SAVE_SIZE);
  put32(out, FATIGUE_RECORD_SIZE - 4, crc(out, FATIGUE_RECORD_SIZE - 4));
}
inline bool matchFatigue(const uint8_t* bytes, const uint8_t* save,
                         uint8_t slot, uint8_t side) {
  return !memcmp(bytes, "FAT6", 4) && bytes[4] == 1 && bytes[5] == slot &&
    bytes[6] == side && bytes[7] <= 100 &&
    get32(bytes, FATIGUE_RECORD_SIZE - 4) == crc(bytes, FATIGUE_RECORD_SIZE - 4) &&
    !memcmp(bytes + 8, save, SAVE_SIZE);
}
}

// The original slot adapter remains unchanged, including primary tombstones.
// Only an explicitly deleted/empty slot can enter replacement cleanup.
template<class Store> struct FatigueSlotBackend : SlotBackend<Store> {
  using Base = SlotBackend<Store>;
  explicit FatigueSlotBackend(Store& store) : Base(store) {}
  const char* fatigueKey(bool side) const {
    static const char* keys[3][2] = {{"fa", "fb"}, {"fa1", "fb1"}, {"fa2", "fb2"}};
    return keys[this->slot < 3 ? this->slot : 0][side];
  }
  rpg::Read readFatigue(bool side, uint8_t* bytes) {
    if (this->slot > 2 || !this->store.ready) return rpg::Read::Error;
    if (this->store.deleted(this->slot)) return rpg::Read::Missing;
    return this->store.readFatigue(fatigueKey(side), bytes);
  }
  bool writeFatigue(bool side, const uint8_t* bytes) {
    return this->slot < 3 && this->store.ready &&
      this->store.writeFatigue(fatigueKey(side), bytes);
  }
  bool prepareReplacement() {
    if (this->slot > 2 || !this->store.ready) return false;
    if (!this->store.deleted(this->slot)) return true;
    // This happens BEFORE staging the new auxiliary, and tombstone stays set.
    if (!this->store.erase(fatigueKey(false)) || !this->store.erase(fatigueKey(true))) return false;
    uint8_t check[rpg::FATIGUE_RECORD_SIZE];
    return this->store.readFatigue(fatigueKey(false), check) == rpg::Read::Missing &&
      this->store.readFatigue(fatigueKey(true), check) == rpg::Read::Missing;
  }
  bool remove() {
    if (!Base::remove()) return false;
    // Deletion is already committed by the existing tombstone. Failed cleanup
    // cannot revive a hero; recreation retries it before staging a new record.
    this->store.erase(fatigueKey(false));
    this->store.erase(fatigueKey(true));
    return true;
  }
};

namespace rpg {
template<class Backend> struct FatigueJournal : Journal<Backend> {
  using Base = Journal<Backend>;
  bool fatigueReset = false;
  explicit FatigueJournal(Backend& io) : Base(io) {}
  Load load(Game& game) {
    fatigueReset = false;
    auto result = Base::load(game);
    game.fatigue = 0;
    if (result != Load::Ok && result != Load::Recovered) return result;
    uint8_t save[SAVE_SIZE], auxiliary[FATIGUE_RECORD_SIZE];
    // Read the actual selected legacy bytes, rather than re-encoding a migrated
    // Game. Legacy saves have no matching auxiliary and start rested.
    auto primary = this->io.read(this->active ? "b" : "a", save);
    auto extra = this->io.readFatigue(this->active, auxiliary);
    if (primary == Read::Ok && extra == Read::Ok &&
        matchFatigue(auxiliary, save, this->io.slot, this->active)) {
      game.fatigue = auxiliary[7];
    } else {
      // A corrupt/missing/stale companion never hides or erases a valid hero.
      // Both missing could mean a legacy hero OR lost auxiliary data. Explain
      // initialization/reset in either case; do not silently erase that nuance.
      fatigueReset = true;
    }
    return result;
  }
  bool save(const Game& game) {
    if (this->blocked || !valid(game) || game.fatigue > 100) return false;
    if (!this->io.prepareReplacement()) return false;
    bool side = this->active == 0;
    uint8_t save[SAVE_SIZE], auxiliary[FATIGUE_RECORD_SIZE], check[FATIGUE_RECORD_SIZE];
    // A failed acknowledgement may conceal a committed primary. Reconcile it
    // BEFORE touching its auxiliary, including a caller's intentional rollback
    // (club admission). Compensation then gets a fresh sequence on the other side.
    auto prior = this->io.read(side ? "b" : "a", save);
    if (prior == Read::Error) return false;
    if (prior == Read::Ok) {
      Game committed; uint32_t sequence = 0;
      auto decoded = decode(save, committed, sequence);
      if (decoded == Decode::Unsupported) return false;
      if (decoded == Decode::Ok && sequence == this->seq + 1) {
        encode(game, sequence, check);
        auto extra = this->io.readFatigue(side, auxiliary);
        bool alreadySaved = !memcmp(save, check, SAVE_SIZE) && extra == Read::Ok &&
          matchFatigue(auxiliary, save, this->io.slot, side) && auxiliary[7] == game.fatigue;
        this->active = side; this->seq = sequence;
        if (alreadySaved) { fatigueReset = false; return true; }
        side = !side;
      } else if (decoded == Decode::Ok && int32_t(sequence - this->seq) > 0) {
        // Unexpected newer data is not an inactive old copy. Reload before saving.
        return false;
      }
    }
    encode(game, this->seq + 1, save);
    encodeFatigue(save, this->io.slot, side, game.fatigue, auxiliary);
    if (!this->io.writeFatigue(side, auxiliary)) return false;
    // Deleted slots intentionally hide auxiliary reads. Read the raw companion
    // while their tombstone still protects the not-yet-committed replacement.
    if (this->io.store.readFatigue(this->io.fatigueKey(side), check) != Read::Ok ||
        memcmp(auxiliary, check, FATIGUE_RECORD_SIZE)) return false;
    // The ordinary commit point is a valid primary write; for a replacement it
    // is the later tombstone clear. Base verifies the primary and advances seq
    // only on success. Uncertain previous commits were reconciled above.
    if (!Base::save(game)) return false;
    fatigueReset = false;
    return true;
  }
};
}
