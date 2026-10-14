#define RPG_ENABLE_COOP_TRAINING 1
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#define main existing_pvp_radio_main
#include "fatigue76_club_radio.cpp"
#undef main
#pragma GCC diagnostic pop
int main(){
 reset();assert(coopradio::owner==coopradio::Owner::Pvp);assert(!coopradio::acquire(coopradio::Owner::Training));closeClub();assert(coopradio::owner==coopradio::Owner::None);
 auto before=game;auto writes=nvs.primaryWrites;assert(coopradio::acquire(coopradio::Owner::Training));assert(!openClub());assert(same(game,before)&&nvs.primaryWrites==writes&&!arena.opened&&coopradio::owner==coopradio::Owner::Training);
 coopradio::release(coopradio::Owner::Training);assert(openClub());assert(coopradio::owner==coopradio::Owner::Pvp);closeClub();assert(coopradio::owner==coopradio::Owner::None);
 static_assert(club::VERSION==5&&sizeof(club::Packet)==52,"PvP wire changed");puts("PASS: real ClubRadio refuses training-owned radio before reservation/configuration; normal PvP reopens after release; v5/52 preserved");
}
