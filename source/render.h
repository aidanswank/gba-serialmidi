int get1dFrom2d(int x, int y, int width)
{
    int i;
    return i = x + width*y;
}

const char m7fontset[] = " !\"#$%'()*+,-./0123456789:;=?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]`_";

void setTile4(unsigned short* map_data, int tid, int x, int y, int palBank)
{
    int tile_index = get1dFrom2d(x,y,32);
    map_data[tile_index] = tid | SE_PALBANK(palBank);
}

void font_write4(int x, int y, const char* text, unsigned short* map_data, int palBank)
{
	int textlen = strlen(text);

	for(int i = 0; i < textlen; i++)
	{
		char *e;
		int index;

		e = strchr(m7fontset, text[i]);

		index = (int)(e - m7fontset);

        setTile4(map_data,index,i+x,y,palBank);
	}
}

void place_tiled_rectangle(int width, int height, int offx, int offy, int tid, unsigned short* map_data)
{
    for(int i = 0; i < width; i++)
    {
        for(int j = 0; j < height; j++)
        {
            setTile4(map_data, tid, i+offx, j+offy, 0);
        }
    }
}

unsigned short* map_data = se_mem[30];
unsigned short* map_data2 = se_mem[29];

void screen_init()
{
	int charblock = 0;

	/* load the palette into palette memory*/
	// memcpy16(pal_bg_mem, m7font4bppPal, m7font4bppPalLen/2); // 256 pal size
	memcpy16(pal_bg_mem, m7font4bppPal, 32);
	// memcpy16(pal_bg_mem+32, bubPal, 224);
	// memcpy16(pal_bg_mem, bubPal, 224);

	/* load the tileset into charblocks */
	memcpy16(tile_mem[charblock],m7font4bppTiles,m7font4bppTilesLen/2);
	// memcpy16(tile_mem[1],bubTiles,bubTilesLen/2);

	/* load the map into screenblocks */
	memcpy16(se_mem[30],Tile_Layer_1,1024);

    /* set all control the bits in this register */
    REG_BG0CNT = 
		BG_PRIO(2) |    /* priority, 0 is highest, 3 is lowest */
        BG_CBB(charblock)  |       /* the char block the image data is stored in */
        BG_SBB(30) |       /* the screen block the tile data is stored in */
        BG_4BPP  |       /* color mode */
        BG_REG_32x32 ;        /* bg size */
    
    REG_DISPCNT = DCNT_MODE1 | DCNT_BG0;
  
    pal_bg_mem[0] = CLR_GRAY;
}