#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

// linear interpolation
float lerp(float a, float b, float f)
{
    return a + f * (b - a);
}

// this is for htoa to have a place to store a string
u8 tempStr[32];
// converts hex number to string
u8 *htoa(u32 number)
{
	s8 i, tempNum;
	u8 *str = tempStr;

	*str++ = '0';
	*str++ = 'x';
	for(i = 7; i >= 0; i--)
	{
		tempNum = (number >> (i << 2)) & 15;
		if(tempNum == 0)
			*str++ = '0';
		else if(tempNum < 10)
		{
			*str++ = ((u8)'1') + (tempNum - 1);
		}
		else
		{
			*str++ = ((u8)'A') + (tempNum - 10);
		}
	}
	*str = '\0';

	return tempStr;
}

char* i2hstr(int num)
{
    u8* str = htoa( num );
    u8 len = strlen( str );
    const char *last_two = &str[len-2]; 
    return last_two; 
}

#define REPEAT_BYTE(x) ((x) | ((x) << 8) | ((x) << 16) | ((x) << 24))
