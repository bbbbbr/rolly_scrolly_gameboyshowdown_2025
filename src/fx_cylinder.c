#include <gbdk/platform.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <gb/isr.h>

#include "common.h"
#include "input.h"
#include "fade.h"

#include "showdownlogo.h"
#include "font.h"

#include "hUGEDriver.h"
extern const hUGESong_t beatscribe_unused_song;

// #define LOOP_FOREVER

#define CYLINDER_HEIGHT      (72u)
#define CYLINDER_HEIGHT_HALF (72u / 2)
#define CYLINDER_START        ((DEVICE_SCREEN_PX_HEIGHT - CYLINDER_HEIGHT) / 2)
#define SPR_CYLINDER_Y_CENTER  (CYLINDER_START + CYLINDER_HEIGHT_HALF)

#define SPR_NUM   16u
#define SPR_SCALE_X (DEVICE_SCREEN_PX_WIDTH / SPR_NUM)


static void fx_cylinder_isr_vbl(void);
static void fx_cylinder_isr_lcd(void) __interrupt __naked;

uint8_t __at(0xC800) scanline_scy_offsets[256];
uint8_t __at(0xC900) scanline_bg_pals[256];

SFR __at(0xFF93) start_scx;
SFR __at(0xFF94) start_scy;
SFR __at(0xFF95) counter;
SFR __at(0xFF96) bounce_scy;
SFR __at(0xFF97) cylinder_start_bounce_scy;
// uint8_t start_scx, start_scy, counter, bounce_scy, cylinder_start_bounce_scy;
uint8_t cur_scroll_char = 0;

bool demo_running;
uint8_t audio_fading_out;
uint8_t audio_paused;

uint8_t spr_x[SPR_NUM];

const int8_t scy_offset_table[144] = {  // [256]
    // Full size centered cylinder
    // 0,6,2,2,1,1,0,1,0,1,0,0,0,0,1,0,0,0,0,0,0,-1,0,0,0,0,0,0,-1,0,0,0,0,-1,0,0,0,-1,0,0,-1,0,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,-1,0,0,-1,0,-1,0,-1,0,0,-1,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,-1,0,0,0,-1,0,0,-1,0,0,0,-1,0,0,0,0,-1,0,0,0,0,0,0,-1,0,0,0,0,0,0,1,0,0,0,0,1,0,1,0,1,1,2,2,7

    // Half size centered cylinder
    0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,9,4,2,2,2,1,2,1,1,1,0,1,1,1,0,1,0,1,0,1,0,0,1,0,1,0,0,1,0,0,0,1,0,0,1,-1,1,0,0,1,0,0,0,1,0,0,1,0,1,0,0,1,0,1,0,1,0,1,1,1,0,1,1,1,2,1,2,2,2,4,10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1

    // 1/4 size
    // 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,14,6,4,3,3,3,2,2,2,2,2,2,1,2,2,1,2,1,2,1,2,2,1,2,2,2,2,2,2,3,3,3,4,6,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
};

const int8_t palette_ly_table[144] = {
    // Full size centered cylinder
    // 0xFF,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF

    // Half size cylinder
    // 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xF9,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
    // Manually adjusted half cylinder
    // 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFE,0xF9,0xF9,0xF9,0xF9,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xF9,0xF9,0xF9,0xF9,0xFE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
    // v2 (for BG/FG sprite hiding)
        // BLK, BLK, GRY, WHT
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xBF,0xBF,0xBF,0xBF,0xBF,0xBF,0xBF,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0x6F,0xBF,0xBF,0xBF,0xBF,0xBF,0xBF,0xBF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
        // DRK-GRY, BLK, GRY, WHT
        // 0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xBE,0xBE,0xBE,0xBE,0xBE,0xBE,0xBE,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x1E,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0x6E,0xBE,0xBE,0xBE,0xBE,0xBE,0xBE,0xBE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE,0xFE

    // 1/4 size
    // 0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFE,0xFE,0xFE,0xF9,0xF9,0xF9,0xF9,0xF9,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xE4,0xF9,0xF9,0xF9,0xF9,0xF9,0xFE,0xFE,0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};


// Formula: sin(2*pi*t/T)   Amplitude = 8
const uint8_t sine_bounce[32] = {
     0,     2,     3,     4,     6,     7,     7,
     8,     8,     8,     7,     7,     6,     4,
     3,     2,     0,    -2,    -3,    -4,    -6,
    -7,    -7,    -8,    -8,    -8,    -7,    -7,
    -6,    -4,    -3,    -2 };

// Formula: sin(2*pi*t/T)   Amplitude 54 ((72 + 36) / 2)
const uint8_t oam_y_sine[160] = {
   0,   2,   4,   6,   8,  11,  13,  15,  17,  19,
  21,  23,  25,  26,  28,  30,  32,  33,  35,  37,
  38,  40,  41,  42,  44,  45,  46,  47,  48,  49,
  50,  51,  51,  52,  53,  53,  53,  54,  54,  54,
  54,  54,  54,  54,  53,  53,  53,  52,  51,  51,
  50,  49,  48,  47,  46,  45,  44,  42,  41,  40,
  38,  37,  35,  33,  32,  30,  28,  26,  25,  23,
  21,  19,  17,  15,  13,  11,   8,   6,   4,   2,
   0,  -2,  -4,  -6,  -8, -11, -13, -15, -17, -19,
 -21, -23, -25, -26, -28, -30, -32, -33, -35, -37,
 -38, -40, -41, -42, -44, -45, -46, -47, -48, -49,
 -50, -51, -51, -52, -53, -53, -53, -54, -54, -54,
 -54, -54, -54, -54, -53, -53, -53, -52, -51, -51,
 -50, -49, -48, -47, -46, -45, -44, -42, -41, -40,
 -38, -37, -35, -33, -32, -30, -28, -26, -25, -23,
 -21, -19, -17, -15, -13, -11,  -8,  -6,  -4,  -2 };



const uint8_t audio_fade_steps[] = {0b00000000, // Sound off
                                    0b00010001,
                                    0b00100010,
                                    0b00110011,
                                    // 0b01100110,
                                    // 0b01110111,
                                    // 0b11111111 // Sound on highest
                                    };
#define AUDIO_FADE_BITSHIFT 3u
#define AUDIO_FADE_DONE 0u
#define AUDIO_FADE_OUT_START ((ARRAY_LEN(audio_fade_steps) + 1) << AUDIO_FADE_BITSHIFT) // Intentionally +1 since it decrements first then uses value



#define CR(c) (c - 'A')
#define SP  29u
#define PR  26u

const uint8_t scrollytext_src[] = 
"                " \
"HI ALL, HERES A LIL ROLLY SCROLLY. " \
"ONE SCREEN, JUST SOME FUN FOR THE JAM. " \
"FIXED THE GLITCHES AT THE LAST MOMENT. " \
"SIXTY SEC. MAX SO GOTTA JET. " \
"BYEEEEE..." \
"                ";


uint8_t scrollytext[ARRAY_LEN(scrollytext_src)];

void load_scroll_text(void) {
    uint8_t in_chr, out_chr;

    for (uint8_t c = 0; c < ARRAY_LEN(scrollytext_src); c++) {
        in_chr = scrollytext_src[c];    
        if ((in_chr >= 'A') && (in_chr >= 'A')) {
            out_chr = in_chr - 'A';
        }
        else if (in_chr == '.') out_chr = ('Z' - 'A') + 1u;
        else if (in_chr == '!') out_chr = ('Z' - 'A') + 2u;
        else if (in_chr == ',') out_chr = ('Z' - 'A') + 3u;
        else if (in_chr == ' ') out_chr = ('Z' - 'A') + 4u;
        else if (in_chr == ':') out_chr = ('Z' - 'A') + 5u;
        else if (in_chr == ')') out_chr = ('Z' - 'A') + 6u;
        else if (in_chr == '?') out_chr = ('Z' - 'A') + 7u;
        else out_chr = ('Z' - 'A') + 4u;

        scrollytext[c] = out_chr;
    }
}


uint8_t next_scroll_char(void) {

    uint8_t next_char = scrollytext[cur_scroll_char++];

    // Reached the end of the scroller text
    // Either wrap around or quit
    if (cur_scroll_char == (ARRAY_LEN(scrollytext_src) - 6) ) {
        // Start audio fade a little before the end
        audio_fading_out = AUDIO_FADE_OUT_START;
    } else if (cur_scroll_char >= ARRAY_LEN(scrollytext_src)) {
        #ifdef LOOP_FOREVER
            cur_scroll_char = 0;
        #else
            demo_running = false; // quit
        #endif
    }

    return next_char;
}



void fx_cylinder_setup(void) {
    // Clear screen with known blank tile
    fill_bkg_rect(0,0, DEVICE_SCREEN_BUFFER_WIDTH, DEVICE_SCREEN_BUFFER_HEIGHT, 0); // 2); 
    // Load map and tiles
    set_bkg_data(0, showdownlogo_TILE_COUNT, showdownlogo_tiles);
    set_bkg_tiles(0,0,
                  (showdownlogo_WIDTH / showdownlogo_TILE_W),
                  (showdownlogo_HEIGHT / showdownlogo_TILE_H),
                  showdownlogo_map);

    set_sprite_data(0, font_TILE_COUNT, font_tiles);
    load_scroll_text();

    SWITCH_RAM(0);
    ENABLE_RAM;
    
    // Set up 256 byte aligned LUTs in WRAM
    memcpy(scanline_scy_offsets, scy_offset_table, ARRAY_LEN(scy_offset_table));
    memcpy(scanline_bg_pals,     palette_ly_table, ARRAY_LEN(palette_ly_table));


    start_scx = 0u;
    start_scy = 0u;
    counter = 0u;
    bounce_scy = 0u;
    cylinder_start_bounce_scy = 0u;

    // Load the initial sprites
    for (uint8_t c = 0; c < SPR_NUM; c++) {
        spr_x[c] = c * SPR_SCALE_X;
        set_sprite_tile(c, next_scroll_char());
    }

    // Enable audio output
    NR52_REG = AUDENA_ON;
    NR51_REG = AUDTERM_ALL_LEFT | AUDTERM_ALL_RIGHT;
    NR50_REG = 0x77u;
    hUGE_init(&beatscribe_unused_song);
    audio_fading_out = AUDIO_FADE_DONE;
    audio_paused = false;
}


void audio_update(void) {

    if (!audio_paused)
        hUGE_dosound();

    if (audio_fading_out) {
        audio_fading_out--;
        // TODO: only write this once value, use a second timeout counter or other method
        if ((audio_fading_out >> AUDIO_FADE_BITSHIFT) & 0x01)
            NR50_REG = audio_fade_steps[audio_fading_out >> AUDIO_FADE_BITSHIFT];

        // // If done fading out, stop music from playing
        if (audio_fading_out == AUDIO_FADE_DONE) {
            // Fixme: faint pop at the end
            audio_paused = true;
            NR52_REG = AUDENA_OFF;
        }
    }
}


void fx_cylinder_run(void) {

    fx_cylinder_setup();
    vsync();

    CRITICAL {
        STAT_REG = STATF_MODE00;
        // add_LCD(fx_cylinder_isr_lcd);
        add_VBL(fx_cylinder_isr_vbl);
    }
    set_interrupts(IE_REG | LCD_IFLAG);

    demo_running = true;

    // fade_in(FADE_DELAY_NORM, BG_PAL_TITLE);
    OBP0_REG = DMG_PALETTE(DMG_WHITE, DMG_WHITE, DMG_BLACK, DMG_BLACK);
    OBP1_REG = DMG_PALETTE(DMG_WHITE, DMG_WHITE, DMG_BLACK, DMG_BLACK);

    while (demo_running) {
        vsync();

        // Run audio in the main loop so that it can be interrupted by ISRs
        // and since not much else going on in here
        audio_update();

        UPDATE_KEYS();

        // if (KEY_TICKED(J_START)) {
        //     break;
        // }

        uint8_t y_center = (SPR_CYLINDER_Y_CENTER - bounce_scy) + DEVICE_SPRITE_PX_OFFSET_Y;
        if (sys_time & 0x01u) {
            for (uint8_t c = 0; c < SPR_NUM; c++) {
                // Update X
                uint8_t temp_x = spr_x[c] - 1;
                if (temp_x > DEVICE_SCREEN_PX_WIDTH) {
                    temp_x = DEVICE_SCREEN_PX_WIDTH;
                    set_sprite_tile(c, next_scroll_char());
                }
                // Move sprite and save X
                move_sprite(c, temp_x, oam_y_sine[temp_x]  + y_center);
                spr_x[c] = temp_x;
                if ((temp_x < DEVICE_SCREEN_PX_WIDTH * 0.25) || 
                    (temp_x > DEVICE_SCREEN_PX_WIDTH * 0.75)) {
                    set_sprite_prop(c, S_PRIORITY | S_PALETTE);
                } else {
                    set_sprite_prop(c, 0);
                }
            }
        }
    }

    set_interrupts(IE_REG & ~LCD_IFLAG);
    CRITICAL {
        remove_VBL(fx_cylinder_isr_vbl);
    }
    BGP_REG = DMG_PALETTE(DMG_BLACK, DMG_BLACK, DMG_BLACK, DMG_BLACK);
    OBP0_REG = DMG_PALETTE(DMG_BLACK, DMG_BLACK, DMG_BLACK, DMG_BLACK);
    OBP1_REG = DMG_PALETTE(DMG_BLACK, DMG_BLACK, DMG_BLACK, DMG_BLACK);
    // fade_out(FADE_DELAY_NORM, BG_PAL_TITLE);

    // Turn off and reset sound
    NR52_REG = AUDENA_OFF;
}


static void fx_cylinder_isr_vbl(void) {
    counter++;

    // Scroll the background every other frame
    if (counter & 1) start_scy--;

    // Update SCY vertical since wave bounce
    bounce_scy = sine_bounce[(counter >> 1) & 0x1fu];
    cylinder_start_bounce_scy = CYLINDER_START - bounce_scy;

    // Reset Y scroll in VBlank to current start value
    // SCY_REG = start_scy;
    SCX_REG = start_scx;

    // Set scroll to show map line 0 since first LCD ISR won't
    // fire until the end of map line 0, which would leave
    // a possibly incorrect line showing on scanline 0
    SCY_REG = 0;
}

static void fx_cylinder_isr_lcd(void) __interrupt __naked {
    __asm \

    push af
    push hl

    ldh a, (_LY_REG)              // Current scanline
    ld  l, a                      // Save LY

    ldh a, (_cylinder_start_bounce_scy)           // Add sine bounce offset (+/- origin)

        // if (LY <= _cylinder_start_bounce_scy)
        cp  a, l    
        jr  z,  .bottom_start_cyl
        jr  nc, .outside_cyl        // Try to restore correct scrolling due SCY reset in non-cylinder areas

        // if (LY > (108 + bounce_scy)) ...
        add a, #72  
        cp  a, l
        jr  c, .outside_cyl


    // Apply scroll offset for line
    ldh a, (_bounce_scy)           // Add sine bounce offset (+/- origin)
    
    .inside_cyl:
    add a, l                       // A = has LY + bounce_scy
    ld  l, a

    // Load per- scanline BG Pal table
    ld  h, #(_scanline_bg_pals >> 8)  // High byte of address for SCY offsets LUT (fixed location 256 byte aligned)
    ld a, (hl)                    // Add LUT offset to Scroll Y based indexed based on current LYC value
    // ld a, #0xAA
    ldh (_BGP_REG), a             // Apply the updated scroll value

    // Now look up per-scanline Y scroll delta
    dec h                         // ld  h, #(_scanline_scy_offsets >> 8
    ldh a, (_SCY_REG)             // Get current Scroll Y
    add a, (hl)                   // Add LUT offset to Scroll Y based indexed based on current LYC value

    // Map scanline 0 is special- it's tile has all pixels color 0
    // so that sprites with priority set to hide behind the BG will show.
    // But that means if it shows in the cylinder area it will reveal sprite
    // bits and look glitchy. so try to avoid that.
    //
    // Cutting it very close on timing. Might be improved with a rework of the ISR start
    //
    // Avoid scrolling to scanline 0: if ((SCY + LY) == 0) SCY ++
    ld  l, a                      // A has current scroll delta to be loaded to SCY
    ldh a, (_LY_REG)              // Current scanline
    add a, l                      // Add to current scroll delta

    ld  a, l                      // Restore saved scroll delta
    jr  nz, .done_map_line_0_check
        // Scroll past map line 0 -> line 1
        inc a

    .done_map_line_0_check:
    ldh (_SCY_REG), a             // Apply the updated scroll value

    pop hl
    pop af
    reti

    // Need this to properly mask tile palette to all 0 when not in cylinder for sprite priority showing
    .bottom_start_cyl:

        // Load expected offset for
        ld  a, (_start_scy)    

        ld  l, #-36       // TODO: hardwired this in from line 36 of "Which means required SCY REG", but it should be calced
        add a, l
        ld  l, a

        ldh a, (_bounce_scy)           // Add sine bounce offset (+/- origin)
        add a, l
        ldh (_SCY_REG), a             // Apply the updated scroll value

        // Load All black BG Palette
        ld a, #0xFF // #0xAA // 00 // #0xFF
        ldh (_BGP_REG), a             // Apply the updated scroll value

        pop hl
        pop af
        reti

    // Scroll to line 0 which has tile pixel colors all 0
    // which causes the NO-BG-PRIORITY sprites to be VISIBLE
    .outside_cyl:

        // Calculate scroll delta for Map line 0
        ldh a, (_LY_REG)             // Get current Scroll Y
        cpl                          // LY_REG *= -1
        ldh (_SCY_REG), a            // Apply the updated scroll value

        // Load All black BG Palette
        ld a, #0xFE // FC (white) // FD (lgrey) // FE (d-grey) // FF (black) // #0xAA // 00 // #0xFF
        ldh (_BGP_REG), a             // Apply the updated scroll value

        pop hl
        pop af
        reti
    __endasm;    
}

ISR_VECTOR(VECTOR_STAT, fx_cylinder_isr_lcd)

