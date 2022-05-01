#include "direct_sound.h"

const char *wave_options[4] = { 
    "VOLUME",
    "NOTE",
    "LOOP LEN",
    "REVERSE"
};

int wave_values[4] = {
    64,48,0,0
};

void wave_play()
{
    playSound( 16000*percent_table[ wave_values[1] ], wave_values[0], wave_values[2], (bool)wave_values[3] );
}

void wave_settings()
{
    for(int row = 0; row < 4; row++)
    {        
        const char *last_two = i2hstr( wave_values[ row ] );
        if( row == cursorLocationAtPages[WAVE] ) // last page
        {
            font_write4(1, row+3, wave_options[row], map_data, 1);
            font_write4( 16, 3+row, last_two, map_data, 1 );
        } else {
            font_write4(1, row+3, wave_options[row], map_data, 0);
            font_write4( 16, 3+row, last_two, map_data, 0);
        }
    }
    if(key_hit(KEY_A))
    {
        eprint("play wave test");
        // playSound( 16000*percent_table[ wave_values[1] ], wave_values[0], wave_values[2], (bool)wave_values[3]);
        wave_play();
    }
}