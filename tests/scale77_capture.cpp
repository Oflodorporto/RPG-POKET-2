// Host-only screenshots against the production renderer selected by the caller.
// No candidate-only placement helper is used: the same fixture renders old/new code.
#ifndef SCALE77_RENDER_HEADER
#define SCALE77_RENDER_HEADER "render.cpp"
#endif
#define main scale77_unused_render_regression
#include SCALE77_RENDER_HEADER
#undef main
#include <filesystem>
#include <iomanip>
#include <set>
#include <sstream>

namespace {
std::string number(unsigned n) {
  std::ostringstream text;
  text << std::setfill('0') << std::setw(2) << n;
  return text.str();
}

unsigned homeCity(unsigned id) {
  if (id >= 27 || id == 7 || id == 18 || id == 19) return 3;
  if (id >= 20) return 2;
  if (id >= 14) return id - 14;
  if (id == 4 || (id >= 10 && id <= 13)) return 0;
  return 1;
}

void prepareMenu(bool night) {
  menu = MenuState{};
  menu.art = ArtStatus::Ready;
  menu.clockValid = true;
  menu.dayCycle = true;
  menu.worldPeriod = night ? worldClock::Period::Night : worldClock::Period::Day;
}

rpg::Game hero(unsigned cls) {
  auto g = rpg::create(cls, 42);
  g.race = cls;
  g.shirt = 5;
  g.trousers = 7;
  g.tutorial = true;
  return g;
}

struct Capture {
  std::filesystem::path root;
  std::ofstream manifest, skipped;
  unsigned frames = 0;

  explicit Capture(const char* output) : root(output) {
    std::filesystem::create_directories(root);
    manifest.open(root / "captures.tsv");
    skipped.open(root / "skipped.tsv");
    assert(manifest && skipped);
    manifest << "file\tgroup\ttitle\tenemy_id\tclass\tcity\tperiod\tmode\tspawn\tdistance\n";
    skipped << "mode\tspawn\tenemy_id\tdistance\treason\n";
  }

  void save(const std::string& name, const std::string& group,
            const std::string& title, const rpg::Game& g, const ViewState& v,
            const char* period, int mode = -1, int spawn = -1, int distance = 0) {
    Canvas canvas;
    const auto before = g;
    uint8_t saveBefore[rpg::SAVE_SIZE], saveAfter[rpg::SAVE_SIZE];
    rpg::encode(g, 7, saveBefore);
    render(canvas, g, v, 0);
    rpg::encode(g, 7, saveAfter);
    assert(!memcmp(&before, &g, sizeof(g)));
    assert(!memcmp(saveBefore, saveAfter, sizeof(saveBefore)));
    const auto path = root / (name + ".ppm");
    canvas.save(path.string());
    assert(std::filesystem::file_size(path) > 240 * 320 * 3);
    manifest << name << ".ppm\t" << group << '\t' << title << '\t'
             << unsigned(g.enemyId) << '\t' << unsigned(g.p.cls) << '\t'
             << unsigned(g.city) << '\t' << period << '\t' << mode << '\t'
             << spawn << '\t' << distance << '\n';
    ++frames;
  }
};

void captureOutdoor(Capture& out) {
  for (bool night : {false, true}) {
    const std::string period = night ? "night" : "day";
    for (unsigned id = 0; id < enemyArt::ENEMY_COUNT; ++id) {
      prepareMenu(night);
      auto g = hero(id % 4);
      g.city = homeCity(id);
      g.enemyId = id;
      g.enemyHp = rpg::encounterHp(g, id);
      g.phase = rpg::Phase::Hero;
      ViewState view;
      view.page = Page::Battle;
      out.save("outdoor-" + period + "-foe" + number(id), "outdoor-" + period,
               std::string(rpg::enemySpec(id).name) + " / city" + std::to_string(g.city) +
                   " / class" + std::to_string(g.p.cls),
               g, view, period.c_str());
    }
  }
  // Explicit field-report reproductions, across all four unchanged hero classes.
  for (unsigned id : {9u, 5u, 3u}) {
    for (bool night : {false, true}) for (unsigned cls = 0; cls < 4; ++cls) {
      prepareMenu(night);
      auto g = hero(cls);
      g.city = 1;
      g.enemyId = id;
      g.enemyHp = id == 9 ? 35 : id == 5 ? 42 : rpg::encounterHp(g, id);
      g.phase = rpg::Phase::Hero;
      ViewState view;
      view.page = Page::Battle;
      const std::string period = night ? "night" : "day";
      const std::string label = id == 9 ? "vigia-hp35" : id == 5 ? "espectro-hp42" : "guardiao";
      out.save("reported-" + label + "-" + period + "-class" + std::to_string(cls),
               "reported-" + label,
               std::string(rpg::enemySpec(id).name) + " HP" + std::to_string(g.enemyHp) +
                   " / " + period + " / class" + std::to_string(cls),
               g, view, period.c_str());
    }
  }
}

rpg::Game dungeonFixture(unsigned mode, unsigned cls) {
  auto g = hero(cls);
  g.dungeonFlags = 1;
  g.city = 1;
  g.dungeonLoot = 8;
  if (mode == 1) {
    g.dungeonFlags |= 16;
    g.campaignFlags = 255;
    g.campaignEnding = 2;
    g.dungeonLoot = 192;
  } else if (mode >= 2) {
    g.eventStage = 2;
    g.eventKind = 3;
    // Mode6 captures the second real tier0 roster (wolf at level3+).
    g.eventTier = mode == 6 ? 0 : mode - 2;
    g.city = g.eventTier;
    g.eventLevel = mode == 2 ? 2 : mode == 3 || mode == 6 ? 5 : mode == 4 ? 10 : 20;
    g.dungeonLoot = 0;
    g.eventProgress = 0;
  }
  return g;
}

bool positionFor(rpg::Game& g, const rpg::DungeonSpawn& spawn, unsigned distance) {
  // A real unopened chest can hide a distant boss. Prefer another legal corridor
  // without a foreground pickup; never remove props or alter the authored map.
  for (bool allowForegroundPickup : {false, true}) for (unsigned heading = 0; heading < 4; ++heading) {
    const int x = int(spawn.x) - int(distance) * rpg::dungeonDx[heading];
    const int y = int(spawn.y) - int(distance) * rpg::dungeonDy[heading];
    bool clear = true;
    for (unsigned step = 0; step < distance; ++step) {
      const int px = x + int(step) * rpg::dungeonDx[heading];
      const int py = y + int(step) * rpg::dungeonDy[heading];
      if (rpg::dungeonWall(g, px, py) || rpg::dungeonEnemyAt(g, px, py) >= 0) clear = false;
      if (!allowForegroundPickup && step > 0) {
        for (unsigned item = 0; item < rpg::dungeonPickupCount(g); ++item) {
          const auto pickup = rpg::dungeonPickup(g, item);
          if (!(g.dungeonLoot & (1u << item)) && pickup.floor == spawn.floor &&
              pickup.x == px && pickup.y == py) clear = false;
        }
      }
    }
    if (!clear) continue;
    g.dungeonXY = x | (y << 4);
    rpg::dungeonFace(g, heading);
    return true;
  }
  return false;
}

void captureDungeons(Capture& out) {
  static const char* labels[] = {"original", "island", "expedition0-goblin", "expedition1",
                                "expedition2", "expedition3", "expedition0-wolf"};
  unsigned nearFrames = 0, farFrames = 0;
  for (unsigned mode = 0; mode < 7; ++mode) {
    const auto roster = dungeonFixture(mode, 0);
    unsigned nearInMode = 0, farInMode = 0;
    for (unsigned index = 0; index < rpg::dungeonSpawnCount(roster); ++index) {
      for (unsigned distance : {1u, 2u}) {
        prepareMenu(false);
        auto g = dungeonFixture(mode, (mode + index) % 4);
        const auto spawn = rpg::dungeonSpawn(g, index);
        rpg::dungeonSetFloor(g, spawn.floor);
        // Isolate one genuine spawn without moving or replacing its species.
        g.dungeonEnemies = ((1u << rpg::dungeonSpawnCount(g)) - 1) & ~(1u << index);
        g.enemyId = spawn.id;
        g.enemyHp = rpg::encounterHp(g, spawn.id);
        g.phase = distance == 1 ? rpg::Phase::Hero : rpg::Phase::Home;
        if (!positionFor(g, spawn, distance)) {
          out.skipped << labels[mode] << '\t' << index << '\t' << unsigned(spawn.id)
                      << '\t' << distance << "\tNo straight unobstructed viewpoint in the real map\n";
          continue;
        }
        assert(rpg::dungeonValid(g));
        if (distance == 1) assert(rpg::dungeonEnemyAhead(g) == int(index));
        ViewState view;
        view.page = Page::Dungeon;
        out.save(std::string("dungeon-") + labels[mode] + "-spawn" + number(index) +
                     "-foe" + number(spawn.id) + "-" + std::to_string(distance) + "tile",
                 "dungeon-" + std::to_string(distance) + "tile",
                 std::string(labels[mode]) + " / " + rpg::enemySpec(spawn.id).name +
                     " / floor" + std::to_string(spawn.floor) + " / class" + std::to_string(g.p.cls),
                 g, view, "indoor", mode, index, distance);
        if (distance == 1) { ++nearFrames; ++nearInMode; }
        else { ++farFrames; ++farInMode; }
      }
    }
    assert(nearInMode == rpg::dungeonSpawnCount(roster));
    assert(farInMode > 0);
  }
  printf("Captured %u near and %u far real-spawn dungeon views.\n", nearFrames, farFrames);
}
} // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: scale77_capture OUTPUT_DIRECTORY [ART_PACK]\n");
    return 2;
  }
  const char* packPath = argc == 3 ? argv[2] : "cartao/RPGPOKET/artes.pak";
  std::vector<uint8_t> memory(ART_BYTES);
  artMemory = memory.data();
  std::ifstream pack(packPath, std::ios::binary);
  assert(pack);
  pack.seekg(0, std::ios::end);
  assert(pack.tellg() == std::streamoff(ART_BYTES + 16));
  pack.seekg(16);
  pack.read(reinterpret_cast<char*>(artMemory), ART_BYTES);
  assert(pack.gcount() == ART_BYTES && rpg::crc(artMemory, ART_BYTES) == ART_CRC);
  Capture captures(argv[1]);
  captureOutdoor(captures);
  captureDungeons(captures);
  assert(rpg::crc(artMemory, ART_BYTES) == ART_CRC);
  artMemory = nullptr;
  printf("PASS: %u immutable native240x320 production-render captures; actual art CRC verified.\n", captures.frames);
}
