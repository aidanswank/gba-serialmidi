int cursorX = 0, cursorY = 0;
// const int optionsLen = 5;
int curSelect = 0;

const char *channelNames[4] = { 
    "SQ1",
    "SQ2",
    "WAV",
    "NOI",
};

u8 optionsValues[5]={
    0,
    0,
    0,
    10,
    1,
};

u8 optionsValuesLengths[5]={
    4,
    2,
    7,
    15,
    2,
};

// where cursor is at dif pages
#define numPages 4
int cursorLocationAtPages[numPages]={0,0,0,0};
int* refOptionTable[numPages];