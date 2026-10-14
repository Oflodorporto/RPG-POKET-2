#include "../firmware/RPG_POKET_2/CoopConfig.h"
#include "../firmware/RPG_POKET_2/View.h"
static_assert(RPG_ENABLE_COOP_TRAINING==0,"Local training leaked into default release");
static_assert(club::VERSION==5&&sizeof(club::Packet)==52,"PvP contract changed");
int main(){return 0;}
