/*
   Spaz Attack Copyright 2005 by Howard Uman
   All Rights Reserved

   This is a game for ColecoVision.
*/

#include <coleco.h>
#include <getput1.h>
#include "graphics.h"

/* CONSTANTS */
#define TRUE   1
#define FALSE  0

#define NUM_GUYS 3
#define PLAYER1 0
#define PLAYER2 1
#define HIGHSCORE 2

/* Status of current player */
#define NON_GAME 0
#define PLAYING  1
#define DYING    2
#define DEAD     3

#define CENTERX  15
#define CENTERY  12
#define VERTSTEPS 3

/* States of the bullets - note that colors used are in graphics.h */
#define UNLOADED1      0 /* empty */
#define UNLOADED2      1 /* empty */
#define LOADING        2
#define LOADED         3
#define SHOOTING       4
#define BULLET_STATES  SHOOTING+1
#define BULLET_MAX     7 /* 1 bullet per row restriction */

/* useful structures */
typedef struct {
   byte inUse;
   byte direction;
   char posX;
   char posY;
} bulletData;

/* GLOBALS */
byte numPlayers;
byte guys[2];       /* guys left per player */
byte level[2];      /* level each player is on */
score_t scores[3];  /* scores per player (plus high score) */
bulletData bullets[BULLET_MAX];

/* Player data */
byte joypad;
byte shipPosX;
byte shipPosY;
byte shipAim;
byte shipBulletState;

byte gameStatus;
byte color;

/* UTILITY DEFINES */
/* GetPlayerPad gets the appropriate controller info for the player */
#define GetPlayerPad(player) ((player == PLAYER1) ? joypad_1 : joypad_2)

/* functions used in nmi */
void nmi(void)
{
  update_sound();
}

void flipFontColors(byte reset)
{
   byte randomcolors[8];
   char i;

   if (reset)
      color=0;
   if (++color > 8)
      color = 0;
   if (color == 8)
   {
      for (i=0;i<8; i++)
         randomcolors[i] = get_random();
   }

   for (i='/'; i<'z'; i++)
   {
      if (color == 8)
      {
          change_multicolor(i, &randomcolors);
      }
      else
          change_multicolor(i, coolBlueFont[color]);
   }
   if (color == 8)
      delay(2);
}

void setupScreenAndColors()
{
   /* init screen */
   disable_nmi();
   screen_mode_2_text();
   upload_default_ascii(BOLD);
   cls();
   enable_nmi();
   flipFontColors(TRUE);
}

byte getNumPlayers()
{
   byte kp;

   kp = keypad_1;
   if (kp < 1 || kp > 2)
   {
      kp = keypad_2;
      if (kp < 1 || kp > 2)
         kp=0;
   }
   return kp;
}

void drawScores(byte forceDraw)
{
   byte updateHigh = forceDraw;
   /* track the scores stored to not update on duplicates */
   static score_t scoresStored[2] = {{5,5}, {5,5}};
   /* player 1 */

   if ((numPlayers>0) &&
       (forceDraw || !score_cmp_equ(&scores[PLAYER1], &scoresStored[PLAYER1])))
   {
      print_at(0,0,score_str(&scores[PLAYER1],6));
      scoresStored[PLAYER1].lo = scores[PLAYER1].lo;
      scoresStored[PLAYER1].hi = scores[PLAYER1].hi;
      if (score_cmp_gt(&scores[PLAYER1], &scores[HIGHSCORE])) {
         scoresStored[HIGHSCORE].lo = scores[PLAYER1].lo;
         scoresStored[HIGHSCORE].hi = scores[PLAYER1].hi;
         updateHigh = TRUE;
      }
   }
   /* player 2 */
   if ((numPlayers>1) &&
       (forceDraw || !score_cmp_equ(&scores[PLAYER2], &scoresStored[PLAYER2])))
   {
      print_at(26,0,score_str(&scores[PLAYER2],6));
      scoresStored[PLAYER2].lo = scores[PLAYER2].lo;
      scoresStored[PLAYER2].hi = scores[PLAYER2].hi;
      if (score_cmp_gt(&scores[PLAYER2], &scores[HIGHSCORE])) {
         scoresStored[HIGHSCORE].lo = scores[PLAYER2].lo;
         scoresStored[HIGHSCORE].hi = scores[PLAYER2].hi;
         updateHigh = TRUE;
      }
   }
   /* high score */
   if (updateHigh)
      print_at(13,0,score_str(&scores[HIGHSCORE],6));
}

void titleScreen()
{
   cls();
   /* title screen */
   center_string(11, "SPAZ ATTAK");
   center_string(19, "Copyright 2006 unHUman Software");
   center_string(20, "         Howard Uman");
   center_string(23, "Select Number of Players [1 / 2]");

   drawScores(TRUE);
   numPlayers = 0;

   do {
      delay(2);
      flipFontColors(FALSE);
      numPlayers = getNumPlayers();
   } while (numPlayers==0);
}

/* Alert the next player that his turn is coming up */
/* player is 0 based */
void notifyPlayer(byte player)
{
   cls();

   drawScores(TRUE);

   /* notify player */
   center_string(11, (player == PLAYER1) ? "ONE UP" : "TWO UP");
   do {
      delay(2);         
      flipFontColors(FALSE);
   } while ((GetPlayerPad(player) & 0xf0) == 0);
}

void setupGraphics(byte player)
{
   int i;
   disable_nmi();

   /* 4 characters used to display the window frame */
   /* hopefully will be able to make fancy color pattern */
   for (i=128; i<132; i++)
   {
      change_pattern(i, frame, 1);
      change_color (i, frame_color, 1);
   }

   /* 4 characters for ship 132-135 - defined in drawShip */

   /* 2 characters for bullets - top & bottom 136-137 */
   change_pattern(136, bulletChars[0], 2);
   change_multicolor(136, shipColorTop[COLOR_LOADED]);
   change_multicolor(137, shipColorBottom[COLOR_LOADED]);

   enable_nmi();
}

void drawScreen(byte player)
{
   char i;
   char j;

   cls();
   flipFontColors(TRUE);
   drawScores(TRUE);
   setupGraphics(player);

   for (i=0;i<32;i++)
   {
       for (j=2; j<=23;j+=3)
       {
          if (j==2 || j==23 || i<14 || i>17)
             put_char(i,j,128+i%4);
       }
   }
}

void drawShip(byte force)
{
   static char alreadyX = -1;
   static char alreadyY = -1;
   static byte alreadySetAim = -1;
   static byte alreadySetBulletState = -1;
   byte loadedLeft = COLOR_LOADED;
   byte loadedRight = COLOR_LOADED;
   char bulletX = 0;

   if (force)
   {
      alreadyX = -1;
      alreadyY = -1;
      alreadySetAim = -1;
      alreadySetBulletState = -1;
   }

   /* reset the aiming if necessary */
   /* track the last aim so we only */
   /* update if necessary */
   if ((alreadySetAim != shipAim) || force)
   {
      change_pattern(132, &shipChars[shipAim][0], 4);
      alreadySetAim = shipAim;
      force = TRUE;
   }

   force = 1;
   /* color the ship properly if loaded or not */
   /* track the last item if the ship was loaded */
   /* so we only recolor if needed */
   if ((alreadySetBulletState != shipBulletState) || force)
   {
      switch (shipBulletState) {
         case UNLOADED1:
         case UNLOADED2:
            loadedLeft = loadedRight = COLOR_UNLOADED;
            break;
         case LOADING:
            loadedLeft = (shipAim == SHIP_LEFT) ? COLOR_UNLOADED : COLOR_LOADED;
            loadedRight = (shipAim == SHIP_LEFT) ? COLOR_LOADED : COLOR_UNLOADED;
            break;
         case LOADED:
            loadedLeft = loadedRight = COLOR_LOADED;
            break;
         case SHOOTING:
            loadedLeft = (shipAim == SHIP_LEFT) ? COLOR_LOADED : COLOR_UNLOADED;
            loadedRight = (shipAim == SHIP_LEFT) ? COLOR_UNLOADED : COLOR_LOADED;
            /* Draw the bullet */
            bulletX = (shipAim == SHIP_LEFT) ? shipPosX-1 : shipPosX+2;
            put_char(bulletX, shipPosY, 136);
            put_char(bulletX, shipPosY+1, 137);
            break;
      }
      change_multicolor(132, &shipColorTop[loadedLeft]);
      change_multicolor(133, &shipColorTop[loadedRight]);
      change_multicolor(134, &shipColorBottom[loadedLeft]);
      change_multicolor(135, &shipColorBottom[loadedRight]);
      alreadySetBulletState = shipBulletState;
      force = TRUE;
   }

   /* only redraw if we have something to draw */
   if ((alreadyX != shipPosX) || (alreadyY != shipPosY) || force)
   {
      put_char(shipPosX, shipPosY, 132);
      put_char(shipPosX+1, shipPosY, 133);
      put_char(shipPosX, shipPosY+1, 134);
      put_char(shipPosX+1, shipPosY+1, 135);
      alreadyX = shipPosX;
      alreadyY = shipPosY;
   }
}

void eraseVertMovement(byte y)
{
   put_char(15, y, 32);
   put_char(16, y, 32);
}

void eraseHorizMovement(byte x, byte y)
{
   put_char(x, y, 32);
   put_char(x, y+1, 32);
}

void updateBullets()
{
   int i;
   char eraseX;
   char writeX;
   bulletData* pBullet;
   for (i=0; i<BULLET_MAX; i++)
   {
      pBullet = &bullets[i];
      if (pBullet->inUse)
      {
         /* erase the trailing part of the bullet */
         if (pBullet->direction == SHIP_LEFT)
         {
            eraseX = pBullet->posX+1;
            /* if the bullet is done, get rid of it */
            if (eraseX == 0)
               pBullet->inUse = FALSE;
            else if (--(pBullet->posX) >= -1)
               writeX = pBullet->posX;
         } else { /* SHIP_RIGHT */
            eraseX = (pBullet->posX)++;
            if (eraseX == 31)
               pBullet->inUse = FALSE;
            else
               writeX = pBullet->posX+1;
         }

         put_char(eraseX, pBullet->posY, 32);
         put_char(eraseX, pBullet->posY+1, 32);

         if (pBullet->inUse && writeX >= 0 && writeX <= 31)
         {
            put_char(writeX, pBullet->posY, 136);
            put_char(writeX, pBullet->posY+1, 137);
         }
      }
   }
}

byte addBullet(char y, byte aim)
{
   bulletData* pBullet = &bullets[y/3-1];
   if (!pBullet->inUse)
   {
      pBullet->inUse = TRUE;
      pBullet->direction = aim;
      pBullet->posY = y;
      if (aim == SHIP_LEFT)
         pBullet->posX = 14;
      else
         pBullet->posX = 16;
      return TRUE;
   }
   return FALSE;
}

/* This is the main game loop */
void playGame(byte player)
{
   char vertMove = 0;
   byte allowMoveRight = FALSE;
   byte allowMoveLeft = FALSE;
   byte allowPlayerMoveUD = 0; /* allow player u/d movement every 2nd frame */
   byte allowPlayerMoveLR = 0; /* allow player l/r movement every 3rd frame */
   byte moved; /* do not allow player that moved horizontally to move vertically */

   memset(bullets, 0, sizeof(bulletData)*BULLET_MAX);
   drawScreen(player);
   shipPosX = CENTERX;
   shipPosY = CENTERY;
   shipAim = SHIP_RIGHT;
   shipBulletState = LOADED;

   /* this triggers game state in NMI to display various things */
   gameStatus = PLAYING;
   drawShip(TRUE);

   while(gameStatus != DEAD)
   {
      moved = FALSE;
      /* get user input */
      joypad = GetPlayerPad(player);

      /* Check if moving Left/Right */
      if (!allowPlayerMoveLR) {
         if (joypad & RIGHT) {
            /* allow moving to right if not traveling vertically and aimed right and
               either allowed to move right or already on the left */
            if (!vertMove && (shipAim == SHIP_RIGHT) && ((allowMoveRight && shipPosX < 30) || shipPosX < CENTERX))
            {
               eraseHorizMovement(shipPosX, shipPosY);
               shipPosX++;
               moved = TRUE;
            }
            shipAim = SHIP_RIGHT;
         }
         else if (joypad & LEFT) {
            /* allow moving to left if not traveling vertically and aimed left and
               either allowed to move left or already on the right */
            if (!vertMove && (shipAim == SHIP_LEFT) && ((allowMoveLeft && shipPosX > 0) || shipPosX > CENTERX))
            {
               eraseHorizMovement(shipPosX+1, shipPosY);
               shipPosX--;
               moved = TRUE;
            }
            shipAim = SHIP_LEFT;
         }
      }
      /* Check if moving Up/Down */
      if (!allowPlayerMoveUD && !moved) {
         /* Move top to bottom in chunks of 3 */
         if (shipPosX == CENTERX)
         {
            /* if not already moving, figure out where we are going */
            if (!vertMove && (joypad & UP) && (shipPosY > 3))
            {
               vertMove = -VERTSTEPS;
            }
            if (!vertMove && (joypad & DOWN) && (shipPosY < 21))
            {
               vertMove = VERTSTEPS;
            }

            /* if we are going somewhere vertically, move there */
            if (vertMove > 0)
            {
               eraseVertMovement(shipPosY);
               shipPosY++;
               vertMove--;
            }
            if (vertMove < 0)
            {
               eraseVertMovement(shipPosY+1);
               shipPosY--;
               vertMove++;
            }
         }
      }

      /* we update all other bullets before we allow us to shoot */
      updateBullets();

      /* Allow shooting if we are on a line that allows shooting */
      if (shipBulletState == LOADED)
      {
         if (!(shipPosY % VERTSTEPS) && (shipPosX == CENTERX) && (GetPlayerPad(player) & FIRE1))
         {
            if (addBullet(shipPosY, shipAim))
               shipBulletState = SHOOTING;
         }
      } else {
         if (++shipBulletState > SHOOTING) shipBulletState = UNLOADED2;
      }

      /* Update the display */
      drawScores(FALSE);
      drawShip(FALSE);

      /* increment counters to allow player to move */
      allowPlayerMoveUD = !allowPlayerMoveUD;
      if (++allowPlayerMoveLR > 3) allowPlayerMoveLR = 0;

      /* wait for the next frame */
      delay(1);
   }

   /* guy just died */
   gameStatus = NON_GAME;
   guys[player]--;
}

/* This controls the game flipping between players, etc */
void gameLoop()
{
    byte turn = PLAYER1;

    guys[PLAYER1] = NUM_GUYS;
    guys[PLAYER2] = (numPlayers > 1) ? NUM_GUYS : 0;
    score_reset(&scores[PLAYER1]);
    score_reset(&scores[PLAYER2]);

    level[PLAYER1] = level[PLAYER2] = 0;
    while ((guys[PLAYER1] > 0) || (guys[PLAYER2] > 0))
    {
       notifyPlayer(turn);
       playGame(turn);

       /* flip to the other guy */
       if (guys[1-turn] != 0)
          turn = 1-turn;
    }
}

void main(void)
{
   score_reset(&scores[HIGHSCORE]);
   gameStatus = NON_GAME;
   numPlayers = 0;

   /* first get of keypads return 0, so just initialize them */
   keypad_1;
   keypad_2;
   joypad_1;
   joypad_2;

   setupScreenAndColors();

   while (1)
   {
      titleScreen();
      gameLoop();
   }
}
