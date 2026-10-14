#pragma once
#include <cstdint>
namespace coopradio {
// Main-loop ownership only. Callbacks never read or mutate this lease.
enum class Owner:uint8_t {None,Pvp,Training};
inline Owner owner=Owner::None;
inline bool acquire(Owner requested){if(requested==Owner::None||owner!=Owner::None)return false;owner=requested;return true;}
inline void release(Owner requested){if(owner==requested)owner=Owner::None;}
}
