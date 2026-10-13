#define main originalTitleTests
#include "title_controller.cpp"
#undef main
int main() {
  using namespace rpg;
  nvs = Store{};
  for (unsigned slot = 0; slot < 3; ++slot) {
    backend.slot = slot; Game ignored;
    assert(journal.load(ignored) == Load::Empty);
    game = create(slot, 77 + slot); game.tutorial = true; game.originStory = true; game.originPage = 8;
    game.fatigue = 40 + slot; assert(journal.save(game));
  }
  selectSlot(0); assert(game.fatigue == 40);
  // Idle time and menus do not mutate fatigue. Shelter works at full HP/MP.
  now += 86400000u; view.page = Page::Home; tapped(120, 290);
  assert(!game.fatigue && game.p.hp == game.p.maxhp && game.p.mp == game.p.maxmp);
  auto rested = nvs.blobs; tapped(120, 290); assert(!game.fatigue && nvs.blobs == rested);
  for (unsigned slot = 1; slot < 3; ++slot) { selectSlot(slot); assert(game.fatigue == 40 + slot); }
  // The actual exploration tap mutates once before SaveError; retry only saves.
  selectSlot(0); view.page = Page::Explore; nvs.fail = true;
  tapped(120, 240); assert(view.page == Page::SaveError && game.fatigue == 4);
  auto pending = game; auto seed = game.randomState;
  assert(!tickIdleClock(now + 70000u, false) && view.page == Page::SaveError);
  tapped(120, 240); assert(game.fatigue == 4 && game.randomState == seed);
  nvs.fail = false; tapped(120, 269);
  assert(view.page != Page::SaveError && game.fatigue == 4 && game.randomState == seed);
  selectSlot(0); assert(game.fatigue == 4 && game.randomState == seed && game.discovery == pending.discovery);
  // Current preflight cancellation is read-only. The retained legacy trip
  // checkpoint still keeps its fatigue penalty/die across retry and reboot.
  game = create(0, 31); game.tutorial = true; game.fatigue = 95; game.rations = 2;
  assert(journal.save(game)); view.page = Page::TravelConfirm; view.tripDestination = 1;
  auto travelBefore = game; auto travelBlobs = nvs.blobs; tapped(65, 290);
  assert(view.page == Page::Map && !memcmp(&game, &travelBefore, sizeof(game)) && nvs.blobs == travelBlobs);
  auto preview = tripSafety(game, 1); assert(!prepareTrip(game, 1)); nvs.fail = true; savedTransition(Page::TravelRoll);
  assert(view.page == Page::SaveError && game.rations == 1 && game.fatigue == 95);
  auto die = game.tripRoll, survival = game.tripSurvival, total = game.tripTotal;
  nvs.fail = false; tapped(120, 269); selectSlot(0);
  assert(game.rations == 1 && game.fatigue == 95 && game.tripRoll == die && game.tripSurvival == survival && game.tripTotal == total);
  unsigned outcomes = 0;
  for (unsigned d = 1; d <= 20; ++d) outcomes += d == 20 || (d != 1 && d + game.tripSurvival + game.tripLuck >= game.tripDifficulty);
  assert(preview == outcomes * 5);
  // Camp may start for fatigue alone. Cancellation does not eat; retries do not
  // eat twice; the completed rest's single mutation is saved by the same path.
  game = create(1, 23); game.tutorial = true; game.fatigue = 80; game.rations = 2;
  assert(journal.save(game)); openCamp(); auto untouched = nvs.blobs;
  tapped(65, 290); assert(game.rations == 2 && game.fatigue == 80 && nvs.blobs == untouched);
  openCamp(); menu.campRation = true; nvs.fail = true; tapped(175, 290);
  assert(view.page == Page::SaveError && game.rations == 1 && game.fatigue == 80);
  nvs.fail = false; tapped(120, 269); selectSlot(0);
  assert(game.rations == 1 && game.fatigue == 80 && game.campStage == 1);
  game.campRoll = 20; assert(acceptCamp(game) && game.campStage == 3 && journal.save(game));
  selectSlot(0); assert(view.page == Page::CampRest && game.fatigue == 80);
  assert(finishCamp(game)); nvs.fail = true; savedTransition(Page::Explore);
  assert(view.page == Page::SaveError && game.fatigue == 0 && game.rations == 1);
  assert(!finishCamp(game)); nvs.fail = false; tapped(120, 269); selectSlot(0);
  assert(!game.fatigue && game.rations == 1 && !game.campStage);
  // Clear notification for a corrupt companion; valid hero still loads intact.
  nvs.blobs[backend.fatigueKey(journal.active)][0] ^= 1;
  selectSlot(0); assert(!game.fatigue && strstr(message, "Fadiga reiniciada"));
  puts("PASS: fatigue76 actual controller, 3 slots, idle/menu/repeated inputs, full-resource shelter, exploration/travel/camp save retries and restart, auxiliary reset notice");
}
