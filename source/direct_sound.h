#define REG_DM3SAD *(volatile u32 *)0x40000D4
#define REG_DM3DAD *(volatile u32 *)0x40000D8
#define REG_DM3CNT_L *(volatile u16 *)0x40000DC
#define REG_DM3CNT_H *(volatile u16 *)0x40000DE

// misc.h
#define DMA_MEMSET32 (0x100 | 0x400 | 0x8000)
inline void Dma3(void *dest, const void *src, u32 count, u16 flags)
{
	REG_DM3SAD = (u32)src;
	REG_DM3DAD = (u32)dest;
	REG_DM3CNT_L = count;
	REG_DM3CNT_H = flags;
}
// #include "nesquik.h"
// #include "kickyoi.h"
#include "piano.h"

#define SND_MAX_CHANNELS 4

typedef struct _SOUND_CHANNEL
{
	s8 *data;
	u32 pos;
	u32 inc;
	u32 vol;
	u32 length;
	u32 loopLength;

} SOUND_CHANNEL;

typedef struct _SOUND_VARS
{
  s8 *mixBufferBase;
  s8 *curMixBuffer;
  u32 mixBufferSize;
  u16 mixFreq;
  u16 rcpMixFreq;
  s16 samplesUntilMODTick;
  u16 samplesPerMODTick;
  u8 activeBuffer;

} SOUND_VARS;

typedef enum _SND_FREQ
{
	SND_FREQ_5734,
	SND_FREQ_10512,
	SND_FREQ_13379,
	SND_FREQ_18157,
	SND_FREQ_21024,
	SND_FREQ_26758,
	SND_FREQ_31536,
	SND_FREQ_36314,
	SND_FREQ_40137,
	SND_FREQ_42048,
	SND_FREQ_43959,

	SND_FREQ_NUM

} SND_FREQ;

// structure to store all the frequency settings we allow
typedef struct _FREQ_TABLE
{
	u16 timer;
	u16 freq;
	u16 bufSize;

} FREQ_TABLE;

static const FREQ_TABLE freqTable[SND_FREQ_NUM] = {
		// timer, frequency, and buffer size for frequencies
		// that time out perfectly with VBlank.
		// These are in the order of the SND_FREQ enum in Sound.h.
		{62610, 5734, 96},
		{63940, 10512, 176},
		{64282, 13379, 224},
		{64612, 18157, 304},
		{64738, 21024, 352},
		{64909, 26758, 448},
		{65004, 31536, 528},
		{65073, 36314, 608},
		{65118, 40137, 672},
		{65137, 42048, 704},
		{65154, 43959, 736}
};

// Globals, as seen in tutorial day 2. In the tutorial, I hardcoded
// SOUND_MAX_CHANNELS to 4, but it's the same ffect either way
SOUND_CHANNEL sndChannel[SND_MAX_CHANNELS];
SOUND_VARS sndVars;
// MOD sndMod;

// This is the actual double buffer memory. The size is taken from
// the highest entry in the frequency table above.
s8 sndMixBuffer[736 * 2] EWRAM_DATA;

// sound register
#define REG_SGFIFOA *(volatile u32 *)0x40000A0

// Call this once at startup
void SndInit(SND_FREQ freq)
{
	s32 i;

	// enable sound
	//   REG_SGCNT0_H = SOUNDA_LOUT | SOUNDA_ROUT | SOUNDA_FIFORESET | SOUNDA_VOL_100;
	REG_SNDDSCNT = SDS_AL | SDS_AR | SDS_ARESET | SDS_A100 | SDS_DMG100;

	//   REG_SGCNT1 = SOUND_ENABLE;
	REG_SNDSTAT = SSTAT_ENABLE;

    // turn on DMG sound, its not "direct sound"
    REG_SNDDMGCNT = SDMG_BUILD_LR(SDMG_SQR1 | SDMG_SQR2 | SDMG_NOISE, 7);

	// clear the whole buffer area
	i = 0;
	Dma3(sndMixBuffer, &i, 736 * 2 / 4, DMA_MEMSET32);
	// for(int i = 0; i < 736 * 2 / 4, DMA_MEMSET32; i++)
	// {
	// 	sndMixBuffer[]
	// }

	// initialize main sound variables
	sndVars.mixBufferSize = freqTable[freq].bufSize;
	sndVars.mixBufferBase = sndMixBuffer;
	sndVars.curMixBuffer = sndVars.mixBufferBase;
	sndVars.activeBuffer = 1; // 1 so first swap will start DMA

	// initialize channel structures
	for (i = 0; i < SND_MAX_CHANNELS; i++)
	{
		sndChannel[i].data = 0;
		sndChannel[i].pos = 0;
		sndChannel[i].inc = 0;
		sndChannel[i].vol = 0;
		sndChannel[i].length = 0;
		sndChannel[i].loopLength = 0;
	}

	// start up the timer we will be using
	REG_TM0D = freqTable[freq].timer;
	//   REG_TM0CNT = TIMER_ENABLE;
	REG_TM0CNT = 0x80;

	// set up the DMA settings, but let the VBlank interrupt
	// actually start it up, so the timing is right
	//   REG_DM1CNT = 0;
	REG_DMA1CNT = 0;
	//   REG_DM1DAD = (u32)&REG_SGFIFOA;
	REG_DMA1DAD = (u32)&REG_SGFIFOA;

} // SndInit

// Call this every frame to fill the buffer. It can be
// called anywhere as long as it happens once per frame.
void SndMix()
{
	s32 i, curChn;

	// If you want to use a higher frequency than 18157,
	// you'll need to make this bigger.
	// To be safe, it would be best to set it to the buffer
	// size of the highest frequency we allow in freqTable
	s16 tempBuffer[304];

	// zero the buffer
	i = 0;
	//   Dma3(tempBuffer, &i, sndVars.mixBufferSize * sizeof(s16) / 4, DMA_MEMSET32);
	// idk why dma writing [0] = 0, [1] = 304, [3] = 0, [4] = 304...etc causes high pitched tone
	// just decided to zero it with a plain loop instead
	for (int i = 0; i < sndVars.mixBufferSize; i++)
	{
		tempBuffer[i] = 0;
	}

	for (curChn = 0; curChn < SND_MAX_CHANNELS; curChn++)
	{
		SOUND_CHANNEL *chnPtr = &sndChannel[curChn];

		// check special active flag value
		if (chnPtr->data != 0)
		{
			// this channel is active, so mix its data into the intermediate buffer
			for (i = 0; i < sndVars.mixBufferSize; i++)
			{
				// mix a sample into the intermediate buffer
				tempBuffer[i] += chnPtr->data[chnPtr->pos >> 12] * chnPtr->vol;
				chnPtr->pos += chnPtr->inc;
				// mgba_printf(MGBA_LOG_DEBUG, "%i", tempBuffer[i]);

				// loop the sound if it hits the end
				if (chnPtr->pos >= chnPtr->length)
				{
					// check special loop on/off flag value
					if (chnPtr->loopLength == 0)
					{
						// disable the channel and break from the i loop
						chnPtr->data = 0;
						i = sndVars.mixBufferSize;
					}
					else
					{
						// loop back
						while (chnPtr->pos >= chnPtr->length)
						{
							chnPtr->pos -= chnPtr->loopLength;
						}
					}
				}
			} // end for i = 0 to bufSize
		}		// end data != 0
	}			// end channel loop

	// now downsample the 16-bit buffer and copy it into the actual playing buffer
	for (i = 0; i < sndVars.mixBufferSize; i++)
	{
		// >>6 to divide off the volume, >>2 to divide by 4 channels
		// to prevent overflow. Could make a define for this up with
		// SOUND_MAX_CHANNELS, but I'll hardcode it for now

		sndVars.curMixBuffer[i] = tempBuffer[i] >> 8;
		// mgba_printf(MGBA_LOG_DEBUG, "%i", tempBuffer[i]);
	}

} // SndMix

//sound control register locations
#define REG_DM1CNT_H *(volatile u16 *)0x40000C6
#define REG_DM1CNT *(volatile u32 *)0x40000C4
#define REG_DM1SAD *(volatile u32 *)0x40000BC

void playSound(int rate, int volume, int loopLen, bool reversed)
{
    // REG_SND1FREQ = SFREQ_RESET | SND_RATE(NOTE_A, 0);

	sndChannel[0].data = 0;
	// set up channel vars

	// Start at the start
	sndChannel[0].pos = 0;

	// Calculate the increment. The piano sample was
	// originally 8363Hz, so I've hardcoded it in here.
	// Later we'll want to make a table of info on samples
	// so it can be done automatically.
	// also, it is 12-bit fixed-point, so shift up before the divide
	sndChannel[0].inc = ((rate) << 12) / 18157;
	// sndChannel[0].inc = 0;
    // eprint("inc%i", rate)

	// Set the volume to maximum
	sndChannel[0].vol = volume;

	// The length of the original sample (also 12-bit fixed-point)
	// This will go into our sample info table too
	sndChannel[0].length = (int)piano_bytes << 12;

	// Set the loop length to the special no-loop marker value
	sndChannel[0].loopLength = loopLen;

	// Set the data. This will actually start the channel up so
	// it will be processed next time SndMix() is called
	sndChannel[0].data = (s8 *)piano;

    if(reversed)
    {
        sndChannel[0].inc = -(sndChannel[0].inc);
        sndChannel[0].pos = (int)piano_bytes << 12;
    }
}


// int mod = 0;
// bool going_up = true;
void SndVSync()
{
	if (sndVars.activeBuffer == 1) // buffer 1 just got over
	{

		// Start playing buffer 0
		REG_DM1CNT = 0;
		REG_DM1SAD = (u32)sndVars.mixBufferBase;
		// REG_DM1CNT_H =	0x40 | 0x200 | 0x400 | 0x400 | 0x8000;
		REG_DM1CNT_H = 0xB640;

		// REG_DM1CNT_H = DMA_DST_FIXED | DMA_REPEAT | DMA_32 | DMA_AT_REFRESH | DMA_ENABLE;

		// Set the current buffer pointer to the start of buffer 1
		sndVars.curMixBuffer = sndVars.mixBufferBase + sndVars.mixBufferSize;
		sndVars.activeBuffer = 0;
	}
	else // buffer 0 just got over
	{
		// DMA points to buffer 1 already, so don't bother stopping and resetting it

		// Set the current buffer pointer to the start of buffer 0
		sndVars.curMixBuffer = sndVars.mixBufferBase;
		sndVars.activeBuffer = 1;
	}

	// // int i = 10;
	// int modscale = 5;
	// int max = 50;
	// // int mod = 0;
	// int init_freq = square1_values[4];
	// if(mod<max)
	// {
	// 	going_up=true;
	// }
	// if(mod>max)
	// {
	// 	going_up=false;
	// }
	// if(going_up){
	// 	mod += modscale;
	// } else {
	// 	mod -= modscale;
	// }
	// REG_SND1FREQ = init_freq + SFREQ_RATE(mod-50);

	square_vibrato();

	// do {
	// 	int modmax = 10;
	// 	int modmin = 0;
	// 	int mod = 0;
	// 	int modscale = 3;
	// 	while(mod<modmax)
	// 	{
	// 		REG_SND1FREQ = init_freq - SFREQ_RATE(mod);
	// 		mod += modscale;
	// 		// VBlankIntrWait();
	// 	}
	// 	while(mod>modmin)
	// 	{
	// 		REG_SND1FREQ = init_freq - SFREQ_RATE(mod);
	// 		mod -= modscale;
	// 		// VBlankIntrWait();
	// 	}

	// 	i-=1;
	// } while (i>0);

}