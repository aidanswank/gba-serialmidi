#include <string.h>
#include <tonc.h>

#include "circular_buffer.h"
#include "console.h"
#include "uart.h"

#include <stdio.h>
#include <math.h>

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

#define REPEAT_BYTE(x) ((x) | ((x) << 8) | ((x) << 16) | ((x) << 24))

#include "mgba.h"
#define eprint(...) mgba_printf(MGBA_LOG_DEBUG, __VA_ARGS__);

// I think the FIFO is completely transparent to the user code, compared to no
// FIFO. It basically buys you 3 extra sent chars worth of cycles.
void toggle_fifo() {
  REG_SIOCNT & SIO_USE_FIFO ?
      write_console_line("disabling fifo\n") :
      write_console_line("enabling fifo\n");

  // disabling the fifo will reset it
  REG_SIOCNT ^= SIO_USE_FIFO;
}

void help() {
  write_console_line("\nSTART: display help text\n");
  write_console_line("SELECT: toggle fifo\n");
  write_console_line("LEFT: set passthrough IRQ mode\n");
  write_console_line("RIGHT: set Gbaser IRQ mode\n");
  write_console_line("A to write to screen\n");
  write_console_line("B to send over uart\n");
  write_console_line("L to print SIOCNT\n");
  write_console_line("R to print RCNT\n\n");
}

int freq2rate(float f)
{
  int r = ((2048*f)-131072)/f;
  return r;
}

unsigned short current_duty = SSQR_DUTY1_4;

void do_keys() {
  key_poll();

    if(key_hit(KEY_A)) {
      current_duty = SSQR_DUTY3_4;
      write_console_line("That tickles!\n");
    }
    if(key_hit(KEY_B)) {
      current_duty = SSQR_DUTY1_8;
      snd_uart_ret("some data\n", 10);
    }
    if(key_hit(KEY_LEFT)) {
      write_console_line("passthrough uart irq mode set\n");
      irq_add(II_SERIAL, handle_uart_ret);
    }
    if(key_hit(KEY_RIGHT)) {
      write_console_line("Gbaser uart irq mode set\n");
      irq_add(II_SERIAL, handle_uart_gbaser);
    }
    if(key_hit(KEY_SELECT)) {
      toggle_fifo();
    }
    if(key_hit(KEY_START)) {
      help();
    }
    if(key_hit(KEY_L)) {
      print_register(&siocnt, REG_SIOCNT);
    }
    if(key_hit(KEY_R)) {
      print_register(&rcnt, REG_RCNT);
    }
}

#include "lookup.h"

void play_midi_event(u8* event)
{
  // double f = midi2Freq(event[1]);
  // int rate = freq2rate(f);
  float velocity = event[2];
  velocity = velocity/15;
  if(event[0]==144)
  {
    REG_SND1CNT= SSQR_ENV_BUILD((int)velocity, 0, 2) | current_duty;
    int r = rate_table[ event[1] ];
    REG_SND1FREQ = SFREQ_RESET | r;
  }
}

void print_num(int num)
{
  char str[3] = "NNN";
  sprintf(str, "%d", num);
  for(int i=0; i < 3; i++)
  {
    write_char((u32)str[i]);
  }
  write_console_line(" ");
}

////// NOISE GEN STUFF???
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

s32 main() {
  // set up display
	REG_DISPCNT= DCNT_MODE0 | DCNT_BG0;
  
	// Base TTE init for tilemaps
	tte_init_se(
		0,						        // Background number (0-3)
		BG_CBB(0)|BG_SBB(31),	// BG control
		0,					        	// Tile offset (special cattr)
		CLR_WHITE,		        // Ink color
		14,						        // BitUnpack offset (on-pixel = 15)
		NULL,			        		// Default font (sys8) 
		NULL);					      // Default renderer (se_drawg_s)

	// Initialize sprites (outside of screen)
	OBJ_ATTR obj_attr = {160, 240, 0, 0};
	for(int i=0; i<128; i++) oam_mem[i] = obj_attr;

  // Set to UART mode
  init_circ_buff(&g_uart_rcv_buffer, g_rcv_buffer, UART_RCV_BUFFER_SIZE);
  init_uart(SIO_BAUD_115200);

  // set irqs
	irq_init(NULL);
  irq_add(II_SERIAL, handle_uart_gbaser);
	irq_add(II_VBLANK, NULL);

  // welcome text
  write_console_line(".. and now we wait ..\n\n\n");
  write_console_line("ready to receive from uart\n");
  write_console_line("uart mode: Gbaser\n");
  help();

  u8 mynum = 100;
  // eprint( "p = %p\n", (void *) mynum );
  write_console_line(htoa((u32)(void *) mynum));
  char str[3];
  sprintf(str, "%d", mynum);
  write_char(" ");
  write_console_line(str);

  int ptick = 0;
  u8 midi_event[3];

  // main loop
	while(1)
	{
		VBlankIntrWait();
    // do_keys();
    key_poll();

    // if bytes > 0
    if(circ_bytes_available(&g_uart_rcv_buffer)) {
      // write_console_line_circ(&g_uart_rcv_buffer);
      char out;
      while (read_circ_char(&g_uart_rcv_buffer, &out))
      {

        u8 num = REPEAT_BYTE(out);

        //filter out zeros
        if(num!=0)
        {

          midi_event[ptick % 3] = num;

          ptick++;

          if(ptick%3==0)
          {
            ptick=0;
            for(int i = 0; i < 3; i++)
            {
              print_num(midi_event[i]);
            }
            write_console_line("\n");
            play_midi_event(midi_event);
          }

          // print_num(num);
          // vector_add(&midi_msg, num);
          // write_console_line("size ");
          // print_num(midi_msg.total);
          // write_console_line("~ ");

        }

      }
    }
  }
}
