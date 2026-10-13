#define main originalRenderTests
#include "render.cpp"
#undef main

// Reuse the firmware's raster canvas and add assertions for complete UI copy.
struct FatigueCanvas : Canvas {
  std::vector<std::string> labels;

  void print(const char* text) {
    labels.emplace_back(text);
    Canvas::print(text);
  }

  bool has(const std::string& text) const {
    return std::find(labels.begin(), labels.end(), text) != labels.end();
  }
};

static void checkFrame(FatigueCanvas& canvas, const rpg::Game& game,
                       const ViewState& view, unsigned frame) {
  uint8_t before[rpg::SAVE_SIZE], after[rpg::SAVE_SIZE];
  uint8_t runtime[sizeof(game)];
  rpg::encode(game, 17, before);
  memcpy(runtime, &game, sizeof(game));
  const auto* art = artMemory;
  canvas.labels.clear();
  render(canvas, game, view, frame);
  rpg::encode(game, 17, after);
  assert(!memcmp(before, after, sizeof(before)));
  assert(!memcmp(runtime, &game, sizeof(game))); // Includes fatigue and RNG.
  assert(artMemory == art);

  char text[64];
  const char* bands[] = {"Descansado", "Leve", "Cansado", "Muito cansado"};
  unsigned band = game.fatigue < 40 ? 0 : game.fatigue < 70 ? 1 :
                  game.fatigue < 90 ? 2 : 3;
  snprintf(text, sizeof(text), "Fadiga %u/100 / %s", game.fatigue, bands[band]);
  if(view.page!=Page::TravelConfirm)assert(canvas.has(text));
  for (const auto& label : canvas.labels) assert(label.find("-2") == std::string::npos);

  if (view.page == Page::Home) {
    assert(canvas.has("Descanso gratis: fadiga volta a 0"));
    assert(canvas.has("Mapa") && canvas.has("Descanso") && canvas.has("Menu"));
  } else if (view.page == Page::CampSetup) {
    assert(canvas.has(menu.campRation ? "Recupera 100% de HP e MP" :
                                     "Recupera ate 50% de HP e MP"));
    assert(canvas.has(menu.campRation ? "Fadiga: 0 apos descansar" :
                                     "Fadiga: -50 apos descansar"));
    assert(canvas.has("Voltar") && canvas.has("Acampar"));
  } else {
    snprintf(text, sizeof(text), "Partida: %u%% segura / CD %u",
             rpg::tripSafety(game, view.tripDestination),
             rpg::routeDifficulty(game.city, view.tripDestination));
    assert(canvas.has(text));
    snprintf(text, sizeof(text), "%u dias / %u pernoites",
             rpg::dailyTripDays(game.city, view.tripDestination),
             rpg::dailyTripDays(game.city, view.tripDestination)-1);
    assert(canvas.has(text));
    snprintf(text, sizeof(text), "Precisa %u / na bolsa %u",
             rpg::dailyTripDays(game.city, view.tripDestination), game.rations);
    assert(canvas.has(text) && canvas.has("Uma racao por dia de viagem."));
    assert(canvas.has("Cancelar") && canvas.has("Viajar"));
  }
  if (*view.message) assert(canvas.has(view.message));
}

int main(int argc, char** argv) {
  assert(argc == 2);
  artMemory = static_cast<uint8_t*>(malloc(ART_BYTES));
  assert(artMemory);
  makeFallback();
  std::ifstream pack("outputs/RPG_POKET_2_0_Waveshare/cartao_preparado/RPGPOKET/artes.pak",
                     std::ios::binary);
  if (!pack) pack.open("cartao/RPGPOKET/artes.pak", std::ios::binary);
  assert(pack);
  pack.seekg(16);
  pack.read(reinterpret_cast<char*>(artMemory), ART_BYTES);
  assert(pack.gcount() == ART_BYTES && rpg::crc(artMemory, ART_BYTES) == ART_CRC);

  FatigueCanvas canvas;
  const std::string root = argv[1];
  unsigned frames = 0;
  for (unsigned cls = 0; cls < 4; ++cls) {
    for (unsigned city = 0; city < 4; ++city) {
      for (unsigned level : {1u, 20u}) {
        for (unsigned fatigue : {0u, 39u, 40u, 69u, 70u, 89u, 90u, 100u}) {
          auto game = rpg::create(cls, 76);
          game.city = city;
          game.p.xp = rpg::dndXp[level - 1];
          rpg::levelUp(game);
          game.fatigue = fatigue;
          game.p.gold = 999999;
          game.p.hp = 1;
          game.p.mp = 0;
          game.rations = game.charts = game.charms = 9;
          game.sleepKit = true;
          menu = MenuState{};
          ViewState view;

          for (bool message : {false, true}) {
            view.message = message ? "Descanso concluido" : "";
            view.page = Page::Home;
            for (unsigned frame : {0u, 7u}) {
              checkFrame(canvas, game, view, frame);
              ++frames;
            }
            if (!message && cls == 0 && level == 1 && (city == 1 || fatigue == 100))
              canvas.save(root + "/home-city" + std::to_string(city) + "-fatigue" +
                          std::to_string(fatigue) + ".ppm");

            view.page = Page::CampSetup;
            view.message = message ? "Sem racao na bolsa" : "";
            for (bool food : {false, true}) {
              for (bool kit : {false, true}) {
                menu.campRation = food;
                menu.campKit = kit;
                for (unsigned frame : {0u, 7u}) {
                  checkFrame(canvas, game, view, frame);
                  ++frames;
                }
                if (cls == 0 && level == 1 && fatigue == 100 && kit &&
                    (!message || city == 1))
                  canvas.save(root + "/camp-city" + std::to_string(city) + "-food" +
                              std::to_string(food) + "-message" +
                              std::to_string(message) + ".ppm");
              }
            }

            view.page = Page::TravelConfirm;
            view.message = message ? "Termine a acao atual" : "";
            for (unsigned destination = 0; destination < 4; ++destination) {
              if (destination == city) continue;
              view.tripDestination = destination;
              for (bool supplies : {false, true}) {
                game.rations = game.charts = game.charms = supplies ? 9 : 0;
                for (unsigned frame : {0u, 7u}) {
                  checkFrame(canvas, game, view, frame);
                  ++frames;
                }
                if (!message && cls == 0 && level == 1 && city == 0 &&
                    destination == 3 && (fatigue == 89 || fatigue == 90 || fatigue == 100))
                  canvas.save(root + "/travel-fatigue" + std::to_string(fatigue) +
                              "-supplies" + std::to_string(supplies) + ".ppm");
              }
            }
          }
        }
      }
    }
  }

  // Exercise the largest existing status values without changing Save28 layout.
  auto game = rpg::create(2, 76);
  game.p.hp = game.p.maxhp = 65535;
  game.p.mp = game.p.maxmp = 119;
  game.p.level = 99;
  game.p.xp = UINT32_MAX;
  game.p.gold = 999999;
  game.fatigue = 100;
  ViewState view;
  view.page = Page::Home;
  checkFrame(canvas, game, view, 0);
  ++frames;
  assert(rpg::crc(artMemory, ART_BYTES) == ART_CRC);
  free(artMemory);
  artMemory = nullptr;
  printf("PASS: fatigue76 %u frames / 3 screens / 4 classes / 4 cities / "
         "8 boundaries; complete text, bounds, Save28 and runtime immutability, "
         "fixed unchanged art buffer.\n", frames);
}
