#pragma once

#define SHARED_KEY_XY           (*(vu16*)0x02FFFFA8)
#define SHARED_TOUCH_X          (*(vu16*)0x02FFFFAA)
#define SHARED_TOUCH_Y          (*(vu16*)0x02FFFFAC)
#define SHARED_POWER_STATE      (*(vu16*)0x02FFFFAE)

// SHARED_KEY_XY raw hinge bit (1 = lid closed, 0 = lid open)
#define SHARED_KEY_XY_LID       (1 << 7)

// SHARED_POWER_STATE bits, written by the arm9 and applied by the arm7 main loop
#define SHARED_POWER_SLEEP      (1 << 0)
