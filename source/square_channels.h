unsigned short dutyCycles[4] = {
    SSQR_DUTY1_8,
    SSQR_DUTY1_4,
    SSQR_DUTY1_2,
    SSQR_DUTY3_4
};

unsigned short sweepValues[3] = {
    SSW_INC,
    SSW_DEC,
    SSW_OFF
};

unsigned short directionModes[2] = {
    SSQR_DEC,
    SSQR_INC
};

const char *square_options[8] = {
    "DUTY CYCLE",
    "VOLUME",
    "INC/DEC",
    "ENV TIME",
    "FREQ",
    "SHORT",
    "VIBRATO",
    "VIB MAX",
};

int square1_values[8] = {
    0,12,0,6,1485,0,14,25,
};

int square2_values[8] = {
    1,12,0,2,1485,0,14,25,
};

void square_play_note(int voice, int duty, int volume, int dir, int time, int freq, int timed)
{
    // to switch voice registers, im not sure why i had to strip the tonc variables down to the hex
    volatile unsigned long* square_voice_control = (volatile unsigned long*)0x4000062; //REG_SND1CNT;;
    volatile unsigned long* square_voice_freq = (volatile unsigned long*)0x4000064; //REG_SND1FREQ;

    if(voice==0)
    {
        // square_voice_control = (volatile unsigned long*)0x4000062; //REG_SND1CNT;
        // square_voice_freq    = (volatile unsigned long*)0x4000064; //REG_SND1FREQ;
        REG_SND1CNT = SSQR_ENV_BUILD(volume, dir, time) | SSQR_DUTY( duty );
        REG_SND1FREQ = SFREQ_RESET | freq | (timed<<14);
    } else {
        // square_voice_control = (volatile unsigned long*)0x4000068; //REG_SND2CNT
        // square_voice_freq    = (volatile unsigned long*)0x400006C; //REG_SND2FREQ
        REG_SND2CNT = SSQR_ENV_BUILD(volume, dir, time) | SSQR_DUTY( duty );
        REG_SND2FREQ = SFREQ_RESET | freq | (timed<<14);
    }

}

int mod = 0;
bool going_up = true;
void square_vibrato()
{
	int modscale = square1_values[6];
	int max = square1_values[7];
    int min = 0;
	int init_freq = square1_values[4];
	if(mod<=min)
	{
		going_up=true;
	}
	if(mod>=max)
	{
		going_up=false;
	}
	if(going_up){
		mod += modscale;
	} else {
		mod -= modscale;
	}
	REG_SND1FREQ = init_freq + SFREQ_RATE(mod-max);
}

void square_play(int voice)
{
    int* sv;
    if(voice==0)
    {
        sv = square1_values;
    } else {
        sv = square2_values; 
    }
    square_play_note(voice,sv[0],sv[1],sv[2],sv[3],sv[4],sv[5]);
}

void square_settings(int voice)
{
    for(int row = 0; row < 8; row++)
    {        
        const char *last_two = i2hstr( refOptionTable[ voice ][ row ] );
        if( row == cursorLocationAtPages[ voice ] ) // 0 = first page 1 = second page
        {
            font_write4(1, row+3, square_options[row], map_data, 1);
            font_write4( 16, 3+row, last_two, map_data, 1 );
        } else {
            font_write4(1, row+3, square_options[row], map_data, 0);
            font_write4( 16, 3+row, last_two, map_data, 0);
        }
    }
    if(key_hit(KEY_A))
    {
        eprint("square %i pressed", voice)
        // font_write4(1, 3,"AHHHHHHH", map_data, 0);
        square_play(voice);
    }
}