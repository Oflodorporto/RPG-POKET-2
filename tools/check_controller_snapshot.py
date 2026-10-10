from pathlib import Path
root=Path('.')
body=(root/'firmware/RPG_POKET_2/RPG_POKET_2.ino').read_text().split('void beginEffect(',1)[1].split('#include "ClubRadio.h"',1)[0]
for name in ['title_controller','dungeon_controller']:
 test=(root/'tests'/f'{name}.cpp').read_text().split('void beginEffect(',1)[1].split('int main(',1)[0]
 assert body.strip()==test.strip(),name
 print('PASS: actual controller matches '+name)
