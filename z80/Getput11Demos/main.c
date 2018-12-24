/************************/
/*       NEW GAME       */
/*  By Daniel Bienvenu  */
/*         2002         */
/************************/
#include <coleco.h>
#include <getput1.h>

/* From title.c */
extern byte title[];

/* From icvgm.c */
extern byte NAMERLE[];
extern byte PATTERNRLE[];
extern byte COLOR[];
extern byte SPATTERNRLE[];

/* From sounds.c */
extern byte gob_sound[];
extern byte gotcha_sound[];
extern byte shoot_sound[];

/* Start Gob Sound */
static void gob (void)
{
 /* Gob sound with priority 7 */
 start_sound (gob_sound,7);
 /* No delay */
}

/* Start Gotcha Sound */
static void gotcha (void)
{
 /* Gotcha sound with priority 7 */
 start_sound (gotcha_sound,7);
 /* Let the sound be played during 20 cycles before doing anything else */
 delay (20);
}

/* Start Shoot Sound */
static void shoot (void)
{
 /* Shoot sound with priority 7 */
 start_sound (shoot_sound,7);
 /* No delay */
}

/* No Maskable Interrupt */
void nmi(void)
{
 /* Play sounds */
 update_sound();
 /* Update sprites in VRAM with sprites[] */
 updatesprites (0,2);
}

/* Intro sub-routine */
void intro(void)
{
 /* Set VDP Mode 2 */
 screen_mode_2_bitmap();
 /* Show title screen */
 show_picture(title);
 /* Enable NMI to hear the sound */
 enable_nmi();
 /* Play Gotcha sound */
 gotcha();
 /* Waiting for a FIRE button */
 pause();
 /* Disable NMI */
 disable_nmi();
}

/* Animation sub-routine */
void animation(void)
{
 /* Define and init x and y variables */
 byte x=0;
 byte y=8;

 /* Initialize Screen and Sprites with tables from icvgm.c */
 screen_mode_2_text();
 /* Disable NMI */
 disable_nmi();
 load_namerle(NAMERLE);
 load_patternrle(PATTERNRLE);
 load_color(COLOR);
 load_spatternrle(SPATTERNRLE);
 sprites[0].pattern = 0;
 sprites[0].colour = 6;
 sprites[1].pattern = 4;
 sprites[1].colour = 4;
 /* Disable NMI */
 enable_nmi();

 /* Move Sprite 0 accros the screen */
 while(x != 241)
 {
  /* Using disable and enable NMI functions here help to prevent a curious refresh bug */
  /* Disable NMI */
  disable_nmi();
  /* Update sprites[0] and sprites[0] values with x and y variables */
  sprites[0].x = x;
  sprites[0].y = y;
  sprites[1].x = x;
  sprites[1].y = y;
  /* Enable NMI */
  enable_nmi();

  /* A little delay of 2 cycles to slowdown the animation */
  delay(1);
  /* Update x and y variables value */
  x++;
  y += (x & 1);
 }
 /* Move sprites[0] off the screen */
 sprites[0].y = 204;
 sprites[1].y = 204;
 /* A little delay to give time to NMI to update sprites in VRAM */
 delay(1);
}

void box(byte x1, byte y1, byte x2, byte y2, byte c, byte full)
{
 byte j,l;
 /* Draw a rectangle game space with clearing screen */
 /* Draw a box */
 l = x2-x1;
 for (j = y1; j <= y2; j++)
 {
  fill_at(x1,j,c,l);
 }
 if (!full)
 {
  l -= 2;
  /* Empty the inside of the box */
  for (j = y1+1; j < y2; j++)
  {
   fill_at(x1+1,j,0x20,l);
  }
 }
}

byte my_random_function(byte min, byte max)
{
 /* Warning : min and max must be positive numbers between 1 and 127 */
 byte i = -1;
 while (i<min || i>max)
 {
  i = get_random() & 0x7f;
 }
 return i;
}

void game(void)
{
 /* Define some games and temporary variables */
 byte i,x,y,c;
 byte crosses = 10;
 byte j, playerx, playery;

 /* Using disable and enable NMI functions here help to prevent a curious refresh bug */
 /* Disable NMI */
 disable_nmi();

 /* Draw a big rectangle of bricks filled with space inside */ 
 box(6,4,26,16,0x70,0);
 /* Draw a small box of bricks inside the big rectangle */
 box(13,9,19,11,0x70,1);

 /* Add some yellow crosses in empty spaces (RANDOM) */
 i = 0;
 /* while there is not enough crosses on screen then loop */
 while (i < crosses)
 {
  /* Choose a random COOR X,Y */
  x = my_random_function(7,25);
  y = my_random_function(5,15);
  c = get_char(x,y);
  /* if it is an empty space then add a cross */
  if (c==0x20)
  {
   put_char(x,y,0x68);
   i++;
  }
 }

 /* Add a blue player in an empty space (RANDOM) */
 c = -1;
 /* while it is not an empty space then loop */
 while (c != 0x20)
 {
  /* Choose a random COOR X,Y */
  playerx = my_random_function(7,25);
  playery = my_random_function(5,15);
  c = get_char(playerx,playery);
 }
 put_char(playerx,playery,0x60);

 /* Enable NMI */
 enable_nmi();

 /* Start Shoot sound for fun */
 shoot();

 /* Game loop */
 while (crosses!=0)
 {
  x = playerx;
  y = playery;

  /* A GENERIC JOYPAD FUNCTION TO AVOID COOR X,Y OUT THE SCREEN */
  j = joypad_1;
  if (((j & 1)!=0) && y!=0) y--;
  if (((j & 2)!=0) && x!=31) x++;
  if (((j & 4)!=0) && y!=23) y++;
  if (((j & 8)!=0) && x!=0) x--;

  /* If player move joystick */
  if (playerx!=x || playery!=y)
  {
   /* Look at the new COOR */
   c = get_char(x,y);
   /* If it's not a brick then updated the playerx and playery values with x and y */
   if (c!=0x70)
   {
    /* Erase the blue player from old COOR */
    put_char(playerx,playery,0x20);
    playerx = x;
    playery = y;
    /* Add the blue player at the new COOR */
    put_char(playerx,playery,0x60);
   }
   if (c==0x68)
   {
    /* Start Gob sound */
    gob();
    /* decrease the number of crosses */
    crosses--;
    /* Note : the cross was already replaced by the player on screen */
   }
  }

  /* A delay to slowdown the game speed */
  delay(5);
 }

 /* A delay to let the last gob sound to be fully played before exit the game sub-routine */
 delay(25);
}

void gameover(void)
{
 /* Using disable and enable NMI functions here help to prevent a curious refresh bug */
 /* Disable NMI */
 disable_nmi();
 /* Clear screen */
 cls();
 /* Print messages on screen */
 center_string(5,"THE GAME IS OVER");
 center_string(20,"PRESS FIRE TO RESTART");
 /* Enable NMI */
 enable_nmi();

 /* Waiting for a FIRE button */
 pause();
}

/* Start routine */
void main(void)
{
 intro();
 animation();
 game();
 gameover();
}