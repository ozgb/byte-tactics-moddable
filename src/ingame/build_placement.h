#ifndef BUILD_PLACEMENT_H
#define BUILD_PLACEMENT_H

struct MouseEvent;

void __stdcall HandleLeftClick(MouseEvent* mouse);
void __stdcall HandleRightClick(MouseEvent* mouse);
void __stdcall BattleFrame(void);
void __stdcall EndGameFrame(void);

#endif
