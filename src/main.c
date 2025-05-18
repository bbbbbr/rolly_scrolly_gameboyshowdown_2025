#include <gbdk/platform.h>
#include <stdint.h>

//#include "common.h"
#include "input.h"
#include "fade.h"

#include "fx_cylinder.h"


static void main_init(void) {

    HIDE_BKG;
    HIDE_SPRITES;
    DISPLAY_ON;
	UPDATE_KEYS();

    fade_out(FADE_DELAY_NORM, BG_PAL_TITLE);
    SHOW_BKG;
    SHOW_SPRITES;
    // fade_in(FADE_DELAY_NORM, BG_PAL_TITLE);    
}


void main(void){

    main_init();

    fx_cylinder_run();

    while (1) {
        vsync();
    }
}
