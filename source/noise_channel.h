// NOISE MACROS
// Click divider frequency (0,1,2..8 = f*2,f,f/2..f/7)
#define SNS_FREQ(i)		(i)

// Counter stages 
#define SNS_CNT_7		(1<<2)
#define SNS_CNT_15		0

// Counter pre-stepper freqency (0..13)
#define SNS_STEP(i)		(i<<3)

// Timed mode
#define SNS_CONTINUOUS	0
#define SNS_TIMED		(1<<14)

// Reset
#define SNS_RESET		(1<<15)

// noise_play_note(12,0,7,8,1,5,0);

const char *noise_options[7] = { 
    "VOLUME",
    "INC/DEC",
    "RELEASE",
    "FREQ",
    "COUNTER",
    "STEP",
    "TIMED",
};

int noise_values[7] = {
    12,0,2,3,4,4,0,
};

void noise_play_note(int volume,int dir,int time,int freq,int counter,int step,int timed)
{
    REG_SND4CNT = SSQR_ENV_BUILD(volume, dir, time);
    REG_SND4FREQ = SNS_RESET | SNS_FREQ(freq) | (counter<<2) | SNS_STEP(step) | (timed<<14);
}

void noise_play_patch()
{
    noise_play_note(noise_values[0],noise_values[1],noise_values[2],noise_values[3],noise_values[4],noise_values[5],noise_values[6]);
}

void noise_play_midi(u8* event)
{
    noise_values[0]=event[2];
    float p = (float)event[1]/127.0;
    float n = p * 64;
    noise_values[3]=(int)n;
    noise_play_patch();
}

void noise_settings()
{
    for(int row = 0; row < 7; row++)
    {        
        const char *last_two = i2hstr( noise_values[ row ] );
        if( row == cursorLocationAtPages[NOISE] ) // last page
        {
            font_write4(1, row+3, noise_options[row], map_data, 1);
            font_write4( 16, 3+row, last_two, map_data, 1 );
        } else {
            font_write4(1, row+3, noise_options[row], map_data, 0);
            font_write4( 16, 3+row, last_two, map_data, 0);
        }
    }
    if(key_hit(KEY_A))
    {
        eprint("play noise test");
        noise_play_note(noise_values[0],noise_values[1],noise_values[2],noise_values[3],noise_values[4],noise_values[5],noise_values[6]);
    }
}