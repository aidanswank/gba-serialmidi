#include <string.h>
#include <tonc.h>

#include "circular_buffer.h"
#include "console.h"
#include "uart.h"

#include <stdio.h>
#include <math.h>

#include "utils.h"

#define REPEAT_BYTE(x) ((x) | ((x) << 8) | ((x) << 16) | ((x) << 24))

#include "mgba.h"

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

// unsigned short current_duty = SSQR_DUTY1_4;

// void do_keys() {
//   key_poll();

//     if(key_hit(KEY_A)) {
//       // current_duty = SSQR_DUTY3_4;
//       write_console_line("That tickles!\n");
//     }
//     if(key_hit(KEY_B)) {
//       // current_duty = SSQR_DUTY1_8;
//       snd_uart_ret("some data\n", 10);
//     }
//     if(key_hit(KEY_LEFT)) {
//       write_console_line("passthrough uart irq mode set\n");
//       irq_add(II_SERIAL, handle_uart_ret);
//     }
//     if(key_hit(KEY_RIGHT)) {
//       write_console_line("Gbaser uart irq mode set\n");
//       irq_add(II_SERIAL, handle_uart_gbaser);
//     }
//     if(key_hit(KEY_SELECT)) {
//       toggle_fifo();
//     }
//     if(key_hit(KEY_START)) {
//       help();
//     }
//     if(key_hit(KEY_L)) {
//       print_register(&siocnt, REG_SIOCNT);
//     }
//     if(key_hit(KEY_R)) {
//       print_register(&rcnt, REG_RCNT);
//     }
// }

#include "lookup.h"

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

// ////// NOISE GEN STUFF???
// // Click divider frequency (0,1,2..8 = f*2,f,f/2..f/7)
// #define SNS_FREQ(i)		(i)

// // Counter stages 
// #define SNS_CNT_7		(1<<2)
// #define SNS_CNT_15		0

// // Counter pre-stepper freqency (0..13)
// #define SNS_STEP(i)		(i<<3)

// // Timed mode
// #define SNS_CONTINUOUS	0
// #define SNS_TIMED		(1<<14)

// // Reset
// #define SNS_RESET		(1<<15)


#include "m7font4bpp.h"

#include "map.h"

enum Channels {
    SQUARE1,
    SQUARE2,
    WAVE,
    NOISE,
    TOTAL,
};

#include "ui_globals.h"
#include "render.h"

#include "square_channels.h"
#include "noise_channel.h"
#include "wave_channel.h"

void Controller()
{
    if(key_is_down(KEY_START))
    {
        if(key_hit(KEY_RIGHT))
        {
            if(cursorX<3)
            {
                cursorX++;
            }
            //     curSelect++;
        }
        if(key_hit(KEY_LEFT))
        {
            if(cursorX>0)
            {
                cursorX--;
            }
            //     curSelect--;
        }
    } else {
        if(!key_is_down(KEY_B))
        {
            if(key_hit(KEY_UP))
            {
                // if(cursorY>0)
                //     cursorY--;
                cursorLocationAtPages[ cursorX ] = cursorLocationAtPages[ cursorX ] - 1;
            }
            
            if(key_hit(KEY_DOWN))
            {
                // if(cursorY<optionsLen-1)
                // cursorY++;
                cursorLocationAtPages[ cursorX ] = cursorLocationAtPages[ cursorX ] + 1;
            }
        } else {
            // editing value
            if(key_hit(KEY_RIGHT))
            {
                // if(optionsValues[ cursorY ] < optionsValuesLengths[ cursorY ]-1){
                //     optionsValues[ cursorY ] += 1;
                //     eprint("cursorY %i", cursorY);
                // }
                refOptionTable[ cursorX ][ cursorLocationAtPages[ cursorX ] ] += 1;
            }
            if(key_hit(KEY_LEFT))
            {
                // if(optionsValues[ cursorY ] > 0){
                //     optionsValues[ cursorY ] -= 1;
                // }
                refOptionTable[ cursorX ][ cursorLocationAtPages[ cursorX ] ] -= 1;
            }
        }
    }
}

s32 main() {

  screen_init();

  // set up display
	REG_DISPCNT= DCNT_MODE0 | DCNT_BG0;
  
	// // Base TTE init for tilemaps
	// tte_init_se(
	// 	0,						        // Background number (0-3)
	// 	BG_CBB(0)|BG_SBB(31),	// BG control
	// 	0,					        	// Tile offset (special cattr)
	// 	CLR_WHITE,		        // Ink color
	// 	14,						        // BitUnpack offset (on-pixel = 15)
	// 	NULL,			        		// Default font (sys8) 
	// 	NULL);					      // Default renderer (se_drawg_s)

	// Initialize sprites (outside of screen)
	OBJ_ATTR obj_attr = {160, 240, 0, 0};
	for(int i=0; i<128; i++) oam_mem[i] = obj_attr;

  // Set to UART mode
  init_circ_buff(&g_uart_rcv_buffer, g_rcv_buffer, UART_RCV_BUFFER_SIZE);
  init_uart(SIO_BAUD_115200);


  // link diff array values up
  refOptionTable[0] = square1_values;
  refOptionTable[1] = square2_values;
  refOptionTable[2] = wave_values;
  refOptionTable[3] = noise_values;

  SndInit(SND_FREQ_18157);

  // set irqs
	irq_init(NULL);
  irq_add(II_SERIAL, handle_uart_gbaser);
  irq_add(II_VBLANK, SndVSync);
	// irq_add(II_VBLANK, NULL);

  // // welcome text
  // write_console_line(".. and now we wait ..\n\n\n");
  // write_console_line("ready to receive from uart\n");
  // write_console_line("uart mode: Gbaser\n");
  // help();

  // u8 mynum = 100;
  // // eprint( "p = %p\n", (void *) mynum );
  // write_console_line(htoa((u32)(void *) mynum));
  // char str[3];
  // sprintf(str, "%d", mynum);
  // write_char(" ");
  // write_console_line(str);

  square_play(0);

  int ptick = 0;
  u8 midi_event[3] = {1,2,3};

  // main loop
	while(1)
	{
		VBlankIntrWait();
    // do_keys();
    key_poll();
    Controller();

    // clear with tile 0 space 17,10 with offset 1,3
    place_tiled_rectangle(17,10,1,3,0,map_data);

    int cursor_sel = cursorX;

    // display channel names
    for(int i = 0; i < 4; i++)
    {
        if(i == cursor_sel)
        {
            font_write4(1 + (4 * i), 1, channelNames[i], map_data, 1);
        } else {
            font_write4(1 + (4 * i), 1, channelNames[i], map_data, 0);
        }
    }

    switch (cursor_sel)
    {
        case 0:
            // sqr1_settings();
            square_settings(0);
        break;
        case 1:
            // font_write4(1, 3, "SQUARE 2", map_data, 0);
            square_settings(1);
        break;
        case 2:
            wave_settings();
        break;
        case 3:
            // font_write4(1, 3, "NOISE", map_data, 0);
            noise_settings();
        break;
    }

    SndMix();

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
            // ptick=0;
            // for(int i = 0; i < 3; i++)
            // {
            //   print_num(midi_event[i]);
            // }
            // write_console_line("\n");
            int control = midi_event[0];
            int note_rate = rate_table[ midi_event[1] ];
            if(control==144){ // "note on" on channel 1
              float velocity = (midi_event[2]/127.0)*12.0;
              square1_values[1] = (int)velocity;
              square1_values[4] = note_rate;
              square_play(0);
              // font_write4(1, 16, "AADFASDFASDFASDF", map_data, 1);
            }
            if(control==145){ // "note on" on channel 2
              float velocity = (midi_event[2]/127.0)*12.0;
              square2_values[1] = (int)velocity;
              square2_values[4] = note_rate;
              square_play(1);
            }
            if(control==146){ // "note on" on channel 3
              float velocity = (midi_event[2]/127.0)*64.0;
              wave_values[0] = (int)velocity;
              wave_values[1] = midi_event[1];
              wave_play();
            }
            if(control==147){ // "note on" on channel 4
              float velocity = (midi_event[2]/127.0)*12.0;
              noise_values[0] = (int)velocity;
              noise_values[3] = midi_event[1];
              noise_play_patch();
            }

          }

        }
      }
    }


    char str1[3] = "NNN";
    sprintf(str1, "%d", midi_event[0]);
    char str2[3] = "NNN";
    sprintf(str2, "%d", midi_event[1]);
    char str3[3] = "NNN";
    sprintf(str3, "%d", midi_event[2]);
    char buf[256];
    snprintf(buf, sizeof(buf), "%s%s%s", str1, str2, str3);

    font_write4(2, 16, buf, map_data, 1);

  }
}
