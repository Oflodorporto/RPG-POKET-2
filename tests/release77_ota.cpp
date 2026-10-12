// Compiled twice by test_release77_ota.py, against unchanged 7.6 and final headers.
// Hardware, network, SD and mbedTLS calls remain the existing host fakes.
#include "Save.h"
#include "AssetCatalog.h"
#include "FakeDevice.h"
enum class ArtStatus { Ready, Missing };
struct { bool scanning=false, connecting=false; ArtStatus art=ArtStatus::Ready; } menu;
#include "UpdateDevice.h"
#include <fstream>
#include <sstream>

namespace {
constexpr uint32_t finalArtBytes=6661440, finalArtCrc=0x0b84d93f;
constexpr unsigned finalArtCount=636;
std::vector<uint8_t> firmware, art, oldArt;
std::map<std::string, std::vector<uint8_t>> protectedFiles;
updater::Plan manifestPlan, adapterPlan;

std::vector<uint8_t> readFile(const char* path) {
  std::ifstream input(path, std::ios::binary);
  assert(input);
  return {std::istreambuf_iterator<char>(input), {}};
}
std::string fakeDigest(const std::vector<uint8_t>& bytes) {
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, bytes.data(), bytes.size());
  uint8_t digest[32];
  mbedtls_sha256_finish(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  char out[65];
  for(unsigned i=0; i<32; ++i) snprintf(out+2*i, 3, "%02x", digest[i]);
  return out;
}
void tag(const char* path, const updater::Package& package) {
  disk[std::string(path)+".tag"]={package.sha, package.sha+64};
}
void preserved() {
  for(const auto& item : protectedFiles) {
    assert(disk.count(item.first) && disk.at(item.first)==item.second);
    assert(std::find(deleted.begin(), deleted.end(), item.first)==deleted.end());
  }
  assert(!openFiles && !otaOpen);
}
void reset() {
  disk=protectedFiles;
  deleted.clear(); ranges.clear(); urls.clear(); replies.clear(); flashed.clear();
  card=true;
  writeFault=renameFault=readFault=beginFault=otaWriteFault=endFault=bootFault=false;
  beginCalls=bootCalls=abortCalls=endCalls=0;
  openFiles=0; otaOpen=false; ESP.restarted=false; SD.freeBytes=1ull<<30;
  updateWorker=updater::Info{}; updateInfo=updater::Info{};
  updatePlan=adapterPlan; WiFi.state=WL_CONNECTED; maxFiles=1;
  menu.art=ArtStatus::Ready;
}
void cachedFirmware() {
  disk["/RPGPOKET/firmware.part"]=firmware;
  tag("/RPGPOKET/firmware.part", updatePlan.firmware);
}
void cached() {
  cachedFirmware();
  disk["/RPGPOKET/artes.part"]=art;
  tag("/RPGPOKET/artes.part", updatePlan.art);
}
void unavailable() { for(unsigned i=0; i<5; ++i) replies.push_back({500, {}}); }
void checkManifest(const std::string& raw) {
  replies.push_back({200, {raw.begin(), raw.end()}});
  checkUpdate();
}
void noBoot() {
  assert(!beginCalls && !bootCalls && !ESP.restarted && flashed.empty());
  preserved();
}
void parserGuards(const std::string& raw, uint32_t minimumPsram) {
  assert(parseUpdate(raw.c_str(), manifestPlan));
  assert(updater::valid(manifestPlan, target.size, ESP.getPsramSize()));
  assert(updater::valid(manifestPlan, target.size, minimumPsram));
  // The shipped parser checks package bytes + framebuffer with a strict '<'.
  // It does not parse minimum_psram_bytes; Python verifies the larger decode budget.
  const uint32_t threshold=manifestPlan.art.bytes+153600;
  assert(!updater::valid(manifestPlan, target.size, threshold));
  assert(updater::valid(manifestPlan, target.size, threshold+1));
  auto p=manifestPlan; p.firmware.bytes=target.size+1;
  assert(!updater::valid(p, target.size, ESP.getPsramSize()));
  p=manifestPlan; p.firmware.bytes=0;
  assert(!updater::valid(p, target.size, ESP.getPsramSize()));
  p=manifestPlan; p.art.bytes=16;
  assert(!updater::valid(p, target.size, ESP.getPsramSize()));
  p=manifestPlan; p.writesSaveFormat=27;
  assert(!updater::valid(p, target.size, ESP.getPsramSize()));
  p=manifestPlan; p.writesSaveFormat=65536;
  assert(!updater::valid(p, target.size, ESP.getPsramSize()));
  for(unsigned format : {1u, 28u}) {
    auto json=cJSON_Parse(raw.c_str()); assert(json);
    cJSON_DeleteItemFromArray(cJSON_GetObjectItemCaseSensitive(json, "reads_save_formats"), format-1);
    char* text=cJSON_PrintUnformatted(json); assert(text);
    p=updater::Plan{}; assert(!parseUpdate(text, p));
    cJSON_free(text); cJSON_Delete(json);
  }
  assert(!parseUpdate("{broken", p));
  reset();
  checkManifest(std::string(4097, ' '));
  assert(updateWorker.state==updater::State::Error); noBoot();
}
} // namespace

int main(int argc, char** argv) {
  assert(argc==9);
  const bool old=!strcmp(argv[1], "old");
  assert(old || !strcmp(argv[1], "new"));
  const auto manifestBytes=readFile(argv[2]);
  const std::string raw(manifestBytes.begin(), manifestBytes.end());
  assert(!raw.empty() && raw.size()<=4096);
  firmware=readFile(argv[3]); art=readFile(argv[4]); oldArt=readFile(argv[5]);
  const uint32_t expectedBuild=uint32_t(std::stoul(argv[7]));
  const uint32_t minimumPsram=uint32_t(std::stoul(argv[8]));
  assert(art.size()==finalArtBytes+16 && rpg::get32(art.data(), 12)==finalArtCount);
  assert(rpg::get32(art.data(), 8)==finalArtCrc);
  assert(oldArt.size()==6991936+16 && rpg::get32(oldArt.data(), 12)==488);
  assert(rpg::get32(oldArt.data(), 8)==0x53dcc3a5);
  if(old) {
    assert(!strcmp(FW_VERSION, "2026.10.12-fatigue76") && FW_BUILD==2026101203u);
    assert(ART_COUNT==488 && ART_CRC==0x53dcc3a5 && ART_BYTES==6991936);
  } else {
    assert(!strcmp(FW_VERSION, argv[6]) && FW_BUILD==expectedBuild);
    assert(ART_COUNT==finalArtCount && ART_CRC==finalArtCrc && ART_BYTES==finalArtBytes);
  }
  static_assert(rpg::CURRENT_SAVE_FORMAT==28 && rpg::SAVE_SIZE==128, "Save28 contract");
  protectedFiles["/RPGPOKET/artes.pak"]=oldArt;
  protectedFiles["/RPGPOKET/artes_53dcc3a5.pak"]=oldArt;
  for(unsigned slot=0; slot<3; ++slot) {
    auto hero=rpg::create(slot, 7700+slot); hero.p.gold=770+slot;
    std::vector<uint8_t> save(rpg::SAVE_SIZE);
    rpg::encode(hero, 100+slot, save.data());
    rpg::Game restored; uint32_t sequence=0;
    assert(rpg::decode(save.data(), restored, sequence)==rpg::Decode::Ok);
    assert(sequence==100+slot && restored.p.gold==770+slot);
    protectedFiles["/sentinel-save28-slot"+std::to_string(slot)]=save;
  }
  initUpdater();
  parserGuards(raw, minimumPsram);
  assert(!strcmp(manifestPlan.version, argv[6]) && manifestPlan.build==expectedBuild);
  assert(manifestPlan.assets==finalArtCount && manifestPlan.artCrc==finalArtCrc);
  assert(manifestPlan.art.bytes==art.size() && manifestPlan.firmware.bytes==firmware.size());
  assert(manifestPlan.saveFormat==7 && manifestPlan.writesSaveFormat==28 && manifestPlan.readsCurrentSaves);

  // First use the exact, unmodified release manifest, including the real hashes.
  reset(); checkManifest(raw);
  assert(updatePlan.assets==finalArtCount && updatePlan.artCrc==finalArtCrc);
  assert(updateWorker.state==(old?updater::State::Available:updater::State::Current));
  assert(updateWorker.artOnly==!old); noBoot();
  if(old) assert(manifestPlan.build>FW_BUILD && manifestPlan.assets!=ART_COUNT);
  else {
    reset(); menu.art=ArtStatus::Missing; checkManifest(raw);
    assert(updateWorker.state==updater::State::Available && updateWorker.artOnly); noBoot();
    // Same-build repair must still reject a mismatched catalog identity.
    auto json=cJSON_Parse(raw.c_str()); assert(json);
    cJSON_ReplaceItemInObject(cJSON_GetObjectItemCaseSensitive(json, "art"), "assets", cJSON_CreateNumber(488));
    char* text=cJSON_PrintUnformatted(json); assert(text);
    reset(); checkManifest(text);
    assert(updateWorker.state==updater::State::Error); noBoot();
    cJSON_free(text); cJSON_Delete(json);
  }

  // Python already checked real SHA-256 on all input bytes. The existing fake
  // mbedTLS adapter has a test digest, so ONLY installation-plan hashes change.
  adapterPlan=manifestPlan;
  strcpy(adapterPlan.firmware.sha, fakeDigest(firmware).c_str());
  strcpy(adapterPlan.art.sha, fakeDigest(art).c_str());
  char finalPath[48]; updater::artPath(finalPath, sizeof(finalPath), finalArtCrc);
  unsigned installCases=0;
  if(old) {
    // A missing, older, corrupt, or forged-count candidate can never select boot.
    for(unsigned fault=0; fault<4; ++fault) {
      reset(); cached();
      if(fault==0) disk.erase("/RPGPOKET/artes.part");
      if(fault==1) disk["/RPGPOKET/artes.part"]=oldArt;
      if(fault==2) disk["/RPGPOKET/artes.part"][art.size()/2]^=1;
      if(fault==3) {
        auto& candidate=disk["/RPGPOKET/artes.part"];
        rpg::put32(candidate.data(), 12, 488);
        strcpy(updatePlan.art.sha, fakeDigest(candidate).c_str());
        tag("/RPGPOKET/artes.part", updatePlan.art);
      }
      unavailable();
      assert(!installUpdate(false)); noBoot();
      assert(!disk.count(finalPath)); ++installCases;
    }
    // Independently exercise CRC after a matching test digest, not just SHA rejection.
    reset(); cached(); disk["/RPGPOKET/artes.part"][art.size()/2]^=1;
    strcpy(updatePlan.art.sha, fakeDigest(disk.at("/RPGPOKET/artes.part")).c_str());
    tag("/RPGPOKET/artes.part", updatePlan.art);
    assert(!installUpdate(false)); noBoot(); assert(!disk.count(finalPath)); ++installCases;

    // Interrupt the new pack after one complete block; retry the actual installer.
    reset(); cachedFirmware();
    replies.push_back({200, art, -2, "", "", 5000});
    for(unsigned i=0; i<4; ++i) replies.push_back({500, {}});
    assert(!installUpdate(false) && updateWorker.canResume);
    assert(disk.at("/RPGPOKET/artes.part").size()==4096); noBoot();
    assert(partialUpdate(updatePlan.art, "/RPGPOKET/artes.part")==4096);
    std::vector<uint8_t> rest(art.begin()+4096, art.end());
    const auto range="bytes 4096-"+std::to_string(art.size()-1)+"/"+std::to_string(art.size());
    replies.push_back({206, rest, -2, range});
    assert(installUpdate(false) && ESP.restarted && beginCalls==1 && bootCalls==1);
    assert(ranges.back()=="bytes=4096-" && flashed==firmware && disk.at(finalPath)==art);
    assert(!disk.count("/RPGPOKET/firmware.part") && !disk.count("/RPGPOKET/artes.part"));
    preserved(); ++installCases;

    reset(); cached();
    assert(installUpdate(false) && ESP.restarted && beginCalls==1 && bootCalls==1);
    assert(flashed==firmware && disk.at(finalPath)==art); preserved(); ++installCases;
  } else {
    // Real same-identity art repair installs/replaces only the verified new pack.
    for(bool corruptExisting : {false, true}) {
      reset(); menu.art=ArtStatus::Missing; cached();
      if(corruptExisting) disk[finalPath]=oldArt;
      assert(installUpdate(true) && ESP.restarted && !bootCalls && !beginCalls);
      assert(flashed.empty() && disk.at(finalPath)==art); preserved(); ++installCases;
    }
  }
  printf("PASS: %s production OTA headers; exact final manifest %s/%u, %u assets; parser/Save28/partition/PSRAM guards; %u focused install cases; old generic + versioned pack and three Save28 sentinels preserved.\n",
         old?"7.6":"final 7.7", manifestPlan.version, manifestPlan.build, manifestPlan.assets, installCases);
  return 0;
}
