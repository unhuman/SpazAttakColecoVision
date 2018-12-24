/* Spaz Attack Copyright 2005-2009 by Howard Uman
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
#define INITIAL_ENEMY_COUNT 3
/* Status of current player */
#define DEAD          0
#define PLAYING       1
#define ADVANCE_LEVEL 2

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
#define NUM_ROWS       7

/* Constants for Enemies */
#define TOTAL_ENEMIES          6
#define ENEMY_NOT_IN_USE       0
#define ENEMY_SHOOTABLE        1
#define ENEMY_INVINCIBLE       2
/* everything > INVINCIBLE is invincible */
#define ENEMY_COUNT_DOWN       150 /* could go up to 255 */


#define X_WING      0
#define TIE_FIGHTER 1
#define ROCKET      2
#define ARROW       3
#define WALKER      4
#define DEATH_EGG   5
#define SNAKE       6
#define TANK        7
#define BURGER      8
byte points[] = {30, 50, 100, 40, 20, 600, 200, 50, 0 };

#define EXPLOSION   255

#define SHIP_SPRITE  8

#define PATTERN_SHIP_COLLISION     0
#define PATTERN_EXPLOSION          4
#define PATTERN_X_WING             8
#define PATTERN_TIE_FIGHTER       12
#define PATTERN_LEFT_TANK         16
#define PATTERN_RIGHT_TANK        20
#define PATTERN_LEFT_ROCKET       24
#define PATTERN_RIGHT_ROCKET      28
#define PATTERN_LEFT_ARROW        32
#define PATTERN_RIGHT_ARROW       36
#define PATTERN_WALKER            40
#define PATTERN_DEATH_EGG         44
#define PATTERN_LEFT_SNAKE        48
#define PATTERN_RIGHT_SNAKE       52
#define PATTERN_BURGER            56

/* useful structures */
typedef struct {
   byte inUse;
   char posX;
   char posY;
} bulletData;


typedef struct {
  byte inUse;
  byte type;
  int velocity;

  /* posXGranular and posX combine to make an "unsigned" which should be adjusted by velocity */
  /* Z80 is low-order byte first */
  byte posXGranular;
  byte posX;

} enemyData;

/* GLOBALS */
byte i, j; /* looping on heap is better than stack */
sprite_t* pSprite;   /* used to optimize access in code */
enemyData* pEnemy;   /* used to optimize access in code */
bulletData* pBullet; /* used to optimize access in code */

byte numPlayers;
byte player;
byte guys[2];       /* guys left per player */
byte level[2];      /* level each player is on */
int levelLength;
unsigned minEnemySpeed;
score_t scores[3];  /* scores per player (plus high score) */
score_t scoresStored[2]; /* track scores against last drawn to prevent wasting time */
int pointBacklog;
bulletData bullets[2][NUM_ROWS];
enemyData enemies[NUM_ROWS];
byte maxEnemiesAllowed;


byte enemyColor;
byte nmiThird;
byte flip2;
byte flip3;
byte flip4;
byte flip5;
byte flip6;
byte animate4[] = {0, 1, 2, 1};


/* Player data */
byte joypad;
byte shipPosX;
byte shipPosY;
byte shipAim;
byte shipBulletState;   /* state of ship shooting bullets */
char allowMoveLeftMin;  /* track how far ship can move side-to-side */
char allowMoveRightMax; /* track how far ship can move side-to-side */

byte gameStatus;
byte color;

/* Forward definitions */
void drawScores(byte forceDraw);
byte checkSprColl(byte spr1, byte spr2);

/* UTILITY DEFINES */
/* GetPlayerPad gets the appropriate controller info for the player */
#define GetPlayerPad(player) ((player == PLAYER1) ? joypad_1 : joypad_2)

/* functions used in nmi */
void nmi()
{
  byte pickAnimate4;
  if (++nmiThird == 3) nmiThird = 0;
  switch (nmiThird) {
    case 0:
      flip2 = 1-flip2;
      flip3 = (flip3 == 2) ? 0 : flip3+1;
      flip4 = (flip4 == 3) ? 0 : flip4+1;
      flip5 = (flip5 == 4) ? 0 : flip5+1;
      flip6 = (flip6 == 5) ? 0 : flip6+1;
      pickAnimate4 = animate4[flip4];
      change_spattern(PATTERN_EXPLOSION, explosionChars[flip2], 4);
      change_spattern(PATTERN_X_WING, xFighterChars[pickAnimate4], 4);
      change_spattern(PATTERN_TIE_FIGHTER, tieFighterChars[pickAnimate4], 4);
      change_spattern(PATTERN_LEFT_ROCKET, rocketLeftChars[flip3], 4);
      change_spattern(PATTERN_RIGHT_ROCKET, rocketRightChars[flip3], 4);
      change_spattern(PATTERN_WALKER, walkerChars[pickAnimate4], 4);
      change_spattern(PATTERN_DEATH_EGG, deathEggChars[flip6], 4);
      change_spattern(PATTERN_LEFT_SNAKE, snakeLeftChars[pickAnimate4], 4);
      change_spattern(PATTERN_RIGHT_SNAKE, snakeRightChars[pickAnimate4], 4);
      change_spattern(PATTERN_LEFT_TANK, tankLeftChars[flip2], 4);
      change_spattern(PATTERN_RIGHT_TANK, tankRightChars[flip2], 4);
      change_spattern(PATTERN_BURGER, burgerChars[pickAnimate4], 4);
      break;
    case 1:
      drawScores(FALSE);
      break;
  }

  update_sound();
  updatesprites(0, 8);
}

void flipFontColors(byte reset)
{
   byte randomcolors[8];

   disable_nmi();

   if (reset)
      color=0;
   if (++color > 8)
      color = 0;
   if (color == 8)
   {
      for (i=0;i<8; i++)
         randomcolors[i] = get_random();
   }

   for (i='-'; i<='z'; i++)
   {
      if (color == 8)
      {
          /* this leaves traces under the letter S */
          change_multicolor(i, &randomcolors);
      }
      else {
          change_multicolor(i, coolBlueFont[color]);
      }
   }
   enable_nmi();
   delay(1);
   disable_nmi();
}

void setupScreenAndColors()
{
   /* init screen */
   disable_nmi();
   screen_mode_2_text();
   upload_default_ascii(BOLD);
   sprites_16x16();
   clear_sprites(0,64);
   cls();
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
   /* track the scores stored to not update on duplicates */
   byte updateHigh = forceDraw;

   /* roll backlogged scores */
   if (pointBacklog > 0) {
      pointBacklog -= 10;
      score_add(&scores[player], 10);
   }

   /* player 1 */
   if (forceDraw || !score_cmp_equ(&scores[PLAYER1], &scoresStored[PLAYER1]))
   {
      print_at(0,0,score_str(&scores[PLAYER1],6));
      scoresStored[PLAYER1].lo = scores[PLAYER1].lo;
      scoresStored[PLAYER1].hi = scores[PLAYER1].hi;
      if (score_cmp_gt(&scores[PLAYER1], &scores[HIGHSCORE])) {
         scores[HIGHSCORE].lo = scores[PLAYER1].lo;
         scores[HIGHSCORE].hi = scores[PLAYER1].hi;
         updateHigh = TRUE;
      }
   }

   /* player 2 */
   if ((numPlayers>1) &&
       (forceDraw || !score_cmp_equ(&scores[PLAYER2], &scoresStored[PLAYER2])))
   {
      print_at(26, 0, score_str(&scores[PLAYER2],6));
      scoresStored[PLAYER2].lo = scores[PLAYER2].lo;
      scoresStored[PLAYER2].hi = scores[PLAYER2].hi;
      if (score_cmp_gt(&scores[PLAYER2], &scores[HIGHSCORE])) {
         scores[HIGHSCORE].lo = scores[PLAYER2].lo;
         scores[HIGHSCORE].hi = scores[PLAYER2].hi;
         updateHigh = TRUE;
      }
   }
   /* high score */
   if (updateHigh || forceDraw) {
      print_at(13,0,score_str(&scores[HIGHSCORE],6));
   }
}

void titleScreen()
{
   cls();
   disable_nmi();
   clear_sprites(0,64);
   flipFontColors(TRUE);

   /* title screen */
   enable_nmi();
   center_string(11, "SPAZ ATTAK");
   center_string(19, "Copyright 2006-2009");
   center_string(20, "Howard Uman");
   center_string(21, "unHUman Software");
   center_string(23, "Select Number of Players [1 / 2]");
   drawScores(TRUE);
   delay(1);
   disable_nmi();

   numPlayers = 0;

   do {
      flipFontColors(FALSE);
      numPlayers = getNumPlayers();
   } while (numPlayers==0);
}

/* Alert the next player that his turn is coming up */
/* player is 0 based */
void notifyPlayer()
{
   char* levelNum;
   char levelText[12];
   clear_sprites(0,64);
   cls();

   flipFontColors(TRUE);

   disable_nmi();
   drawScores(TRUE);

   center_string(9, "PLAYER  ");
   put_char(19, 9, '1' + player);
   center_string(12, "PREPARE FOR");
   i = 0;
   j = 0;
   levelText[i++] = 'L';
   levelText[i++] = 'E';
   levelText[i++] = 'V';
   levelText[i++] = 'E';
   levelText[i++] = 'L';
   levelText[i++] = ' ';
   levelNum = str(level[player]+1);
   while (levelNum[j] == '0') j++;
   do {
      levelText[i++] = levelNum[j++];
   } while (levelNum[j] != 0);
   levelText[i] = 0;
   center_string(15, levelText);
   enable_nmi();
   delay(1);
   do {
      flipFontColors(FALSE);
   } while ((GetPlayerPad(player) & 0xf0) == 0);
}

void setupGraphics()
{
   disable_nmi();

   /* 4 characters used to display the window frame */
   /* hopefully will be able to make fancy color pattern */
   /* HRU - UPDATE to get rid of loop */
   for (i=128; i<132; i++)
   {
      change_pattern(i, frame, 4);
      change_color(i, frame_color, 4);
   }

   /* 4 characters for ship 132-135 - defined in drawShip */

   /* 2 characters for bullets - top & bottom 136-137 */
   change_pattern(136, bulletChars[0], 2);
   change_multicolor(136, shipColorTop[COLOR_LOADED]);
   change_multicolor(137, shipColorBottom[COLOR_LOADED]);

   change_spattern(PATTERN_SHIP_COLLISION, shipCollisionBallChars, 4);
   change_spattern(PATTERN_LEFT_ARROW, arrowLeftChars, 4);
   change_spattern(PATTERN_RIGHT_ARROW, arrowRightChars, 4);

   enable_nmi();
}

void drawScreen()
{
   cls();
   flipFontColors(TRUE);
   drawScores(TRUE);
   setupGraphics();

   disable_nmi();
   for (i=0;i<32;i++)
   {
       for (j=2; j<=23;j+=3)
       {
          if (j==2 || j==23 || i<14 || i>17) {
             put_char(i,j,128+((i>>6)<<6)); /* %4 */
          }
       }
   }
   enable_nmi();
   delay(1);
   disable_nmi();
}

void drawShip()
{
   byte loadedLeft;
   byte loadedRight;
   byte bulletX;

   loadedLeft = loadedRight = COLOR_LOADED;
   bulletX = 0;

   /* reset the aiming if necessary */
   /* track the last aim so we only */
   /* update if necessary */
   change_pattern(132, &shipChars[shipAim][0], 4);

   /* color the ship properly if loaded or not */
   /* track the last item if the ship was loaded */
   /* so we only recolor if needed */
    switch (shipBulletState) {
       case UNLOADED1:
       case UNLOADED2:
          loadedLeft = loadedRight = COLOR_UNLOADED;
          break;
       case LOADING:
          loadedLeft = (shipAim == MY_LEFT) ? COLOR_UNLOADED : COLOR_LOADED;
          loadedRight = (shipAim == MY_LEFT) ? COLOR_LOADED : COLOR_UNLOADED;
          break;
       case LOADED:
          loadedLeft = loadedRight = COLOR_LOADED;
          break;
       case SHOOTING:
          loadedLeft = (shipAim == MY_LEFT) ? COLOR_LOADED : COLOR_UNLOADED;
          loadedRight = (shipAim == MY_LEFT) ? COLOR_UNLOADED : COLOR_LOADED;
          /* Draw the bullet */
          bulletX = (shipAim == MY_LEFT) ? shipPosX-1 : shipPosX+2;
          put_char(bulletX, shipPosY, 136);
          put_char(bulletX, shipPosY+1, 137);
          break;
    }
    change_multicolor(132, &shipColorTop[loadedLeft]);
    change_multicolor(133, &shipColorTop[loadedRight]);
    change_multicolor(134, &shipColorBottom[loadedLeft]);
    change_multicolor(135, &shipColorBottom[loadedRight]);

   /* only redraw if we have something to draw */
    put_char(shipPosX, shipPosY, 132);
    put_char(shipPosX+1, shipPosY, 133);
    put_char(shipPosX, shipPosY+1, 134);
    put_char(shipPosX+1, shipPosY+1, 135);
    sprites[SHIP_SPRITE].y = (shipPosY << 3)-1;
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
   char eraseX;
   char extraEraseX;
   char writeX;

   for (i=MY_LEFT; i<=MY_RIGHT; i++)
   {
      for (j=0; j<NUM_ROWS; j++)
      {
          pBullet = &bullets[i][j];
          if (pBullet->inUse)
          {
             extraEraseX = -1;

             /* pick an enemy to work against */
             pEnemy = &enemies[j];
             pSprite = &sprites[j];
             /* erase the trailing part of the bullet */
             if (i == MY_LEFT) /* left */
             {
                eraseX = pBullet->posX+1;
                /* if the bullet is done, get rid of it */
                if (eraseX == 0)
                   pBullet->inUse = FALSE;
                else if (--(pBullet->posX) >= -1)
                   writeX = pBullet->posX;

                /* check bullet-to enemy collisions */
                if ((pEnemy->inUse == ENEMY_SHOOTABLE) && (pSprite->x < 128) && 
                    (pSprite->x+8 > writeX<<3) && (absdiff(pSprite->x+8, writeX<<3) < 36)) {
                   pBullet->inUse = FALSE;
                   extraEraseX = eraseX-1;
                   if (pSprite->pattern == PATTERN_RIGHT_TANK) {
                     if (pSprite->x >= 5)
                         pSprite->x -= 5;
                     else
                         pSprite->x = 0;
                     pEnemy->posX = pSprite->x;
                     pEnemy->velocity += 8;
                   } else {
                     levelLength--;
                     score_add(&scores[player], points[pEnemy->type]);
                     pSprite->pattern = PATTERN_EXPLOSION;
                     pEnemy->type = EXPLOSION;
                     pEnemy->inUse = ENEMY_INVINCIBLE; /* explosions can't be killed */
                     pEnemy->velocity = -768;
                   }
                }
             } else { /* right */
                eraseX = (pBullet->posX)++;
                if (eraseX == 31)
                   pBullet->inUse = FALSE;
                else
                   writeX = pBullet->posX+1;

                /* check bullet-to enemy collisions */
                if ((pEnemy->inUse == ENEMY_SHOOTABLE) && (pSprite->x > 128) && 
                    (pSprite->x < writeX<<3) && (absdiff(pSprite->x+8, writeX<<3) < 36)) {
                   pBullet->inUse = FALSE;
                   extraEraseX = eraseX+1;
                   if (pSprite->pattern == PATTERN_LEFT_TANK) {
                     if (pSprite->x <= 235)
                         pSprite->x += 5;
                     else
                         pSprite->x = 240;
                     pEnemy->posX = pSprite->x;
                     pEnemy->velocity -= 8;
                   } else {
                     levelLength--;
                     score_add(&scores[player], points[pEnemy->type]);
                     pSprite->pattern = PATTERN_EXPLOSION;
                     pEnemy->type = EXPLOSION;
                     pEnemy->inUse = ENEMY_INVINCIBLE; /* explosions can't be killed */
                     pEnemy->velocity = 768;
                   }
                }
             }

             /* erase trail */
             put_char(eraseX, pBullet->posY, 32);
             put_char(eraseX, pBullet->posY+1, 32);
             if (extraEraseX != -1) {
                put_char(extraEraseX, pBullet->posY, 32);
                put_char(extraEraseX, pBullet->posY+1, 32);
             }

             /* either remove bullet when there's a shot enemy or advance bullet */
             if ((pBullet->inUse) && (writeX >= 0) && (writeX <= 31)) {
                put_char(writeX, pBullet->posY, 136);
                put_char(writeX, pBullet->posY+1, 137);
             }
          }
      }
   }
}

void updateEnemies() {
  byte oldLocation;
  char flipped;
  byte enemyCount;
  unsigned* pLocation;

  enemyColor = enemyColor + 1;
  if (enemyColor > 15)
     enemyColor = 2;

  /* animation done in nmi() */

  /* Update enemies */
  enemyCount = 0;
  for (i = 0; i < NUM_ROWS; i++) {
    pEnemy = &enemies[i];
    if (pEnemy->inUse == ENEMY_NOT_IN_USE)
       continue;
    else
       enemyCount++;

    pSprite = &sprites[i];

    switch (pEnemy->type) {
       case TANK:
          pSprite->colour = colorsGreen[flip3];
          break;
       case ROCKET:
          pSprite->colour = colorsHot[flip5];
          break;
       case DEATH_EGG:
          if (--(pEnemy->inUse) == ENEMY_SHOOTABLE) {
             pEnemy->type = SNAKE;
             pEnemy->inUse = ENEMY_SHOOTABLE;
             pEnemy->velocity = (pSprite->x == 0) ? 3072 : -3072;
             pSprite->pattern = (pSprite->x == 0) ? PATTERN_RIGHT_SNAKE : PATTERN_LEFT_SNAKE;
          }
       default:
          pSprite->colour = enemyColor;
    }

    flipped = 0;
    oldLocation = pSprite->x;

    pLocation = (unsigned*)&pEnemy->posXGranular;
    *pLocation += pEnemy->velocity;

    if (pEnemy->velocity < 0) {
      if (pEnemy->posX > oldLocation) {
        flipped = 1;
        pEnemy->posX = 0;
        pEnemy->posXGranular = 0;
      }
    } else { /* velocity is positive */
      if ((pEnemy->posX < oldLocation) || (pSprite->x > 240)) {
        flipped = 1;
        pEnemy->posX = 240;
        pEnemy->posXGranular = 255;
      }
    }

    pSprite->x = pEnemy->posX;

    if (flipped) {
        pEnemy->velocity = -(pEnemy->velocity);
        switch (pEnemy->type) {
          case ARROW: /* Arrows turn into tanks */
          case TANK:
               pSprite->pattern = (pEnemy->velocity > 0) ? PATTERN_RIGHT_TANK : PATTERN_LEFT_TANK;
               break;
          case ROCKET:
               pSprite->pattern = (pEnemy->velocity > 0) ? PATTERN_RIGHT_ROCKET : PATTERN_LEFT_ROCKET;
               break;
          case SNAKE:
               pSprite->pattern = (pEnemy->velocity > 0) ? PATTERN_RIGHT_SNAKE : PATTERN_LEFT_SNAKE;
               break;
          case BURGER: /* when burger hits the side, it's gone */
          case EXPLOSION: /* when explosion hits the side, it's gone */
               pEnemy->inUse = ENEMY_NOT_IN_USE;
               pSprite->colour = 0;
               pSprite->x = 255;
               enemyCount--;
               break;
        }
    }

    /* check collisions - won't have effect until AFTER frame is rendered */
    if (checkSprColl(SHIP_SPRITE, i) == 1) {
        /* check to see if we caught the death egg, else death. */
        if (pEnemy->inUse > ENEMY_INVINCIBLE) {
          pointBacklog += 600;
          /* start the burger in the other direction */
          pSprite->pattern = PATTERN_BURGER;
          pEnemy->inUse = ENEMY_INVINCIBLE;
          pEnemy->type = BURGER;
          if (pSprite->x == 0) {
            pSprite->x = 240;
            pEnemy->posX = 240;
            pEnemy->posXGranular = 255;
            pEnemy->velocity = -384;
            allowMoveRightMax = CENTERX;
          } else {
            pSprite->x = 0;
            pEnemy->posX = 0;
            pEnemy->posXGranular = 0;
            pEnemy->velocity = 384;
            allowMoveLeftMin = CENTERX;
          }
        } else {
          gameStatus = DEAD;
        }
    }
  }

  /* bail out of level if we're done and nobody is on screen */
  if (levelLength <= 0) {
     if (enemyCount == 0)
        gameStatus = ADVANCE_LEVEL;
     return;
  }

  /* Create enemies if we are not done the level */
  if (enemyCount < maxEnemiesAllowed) {

    /* determine which row the alien fills in */
    i = get_random() % (NUM_ROWS-enemyCount++);

    /* track j here because otherwise we freeze.  FIX */
    j = 0;
    do {
       pEnemy = &enemies[i];
       if (pEnemy->inUse == ENEMY_NOT_IN_USE) {
          pSprite = &sprites[i];
          break;
       } else if (++i == NUM_ROWS)
         i = 0;
       j++;
    } while (j < NUM_ROWS);

    if (j == NUM_ROWS)
       return;

    pEnemy->inUse = ENEMY_SHOOTABLE;
    pEnemy->velocity = ((get_random() >> 7) == 0) ? (get_random() + minEnemySpeed) : -(get_random() + minEnemySpeed);
    pEnemy->type = get_random() % TOTAL_ENEMIES;
    if (pEnemy->velocity < 0) {
       pEnemy->posX = 240;
       pEnemy->posXGranular = 255;
    } else {
       pEnemy->posX = 0;
       pEnemy->posXGranular = 0;
    }
    pSprite->x = pEnemy->posX;
    pSprite->y = (i << 4) + (i << 3)+ 23; /* the same as *24 */
    pSprite->colour = enemyColor;
    switch (pEnemy->type) {
      case X_WING:
           pSprite->pattern = PATTERN_X_WING;
           break;
      case TIE_FIGHTER:
           pSprite->pattern = PATTERN_TIE_FIGHTER;
           break;
      case ROCKET:
           pSprite->pattern = (pEnemy->velocity > 0) ? PATTERN_RIGHT_ROCKET : PATTERN_LEFT_ROCKET;
           break;
      case ARROW:
           pSprite->pattern = (pEnemy->velocity > 0) ? PATTERN_RIGHT_ARROW : PATTERN_LEFT_ARROW;
           break;
      case WALKER:
           pSprite->pattern = PATTERN_WALKER;
           pEnemy->velocity = (pEnemy->velocity > 0) ? 256 : -256;
           break;
      case DEATH_EGG:
           pSprite->pattern = PATTERN_DEATH_EGG;
           pEnemy->inUse = ENEMY_COUNT_DOWN;
           pEnemy->velocity = 0; /* Bonus dots don't move */
           break;
/*    case BURGER:
           pSprite->pattern = PATTERN_BURGER;
           pEnemy->inUse = ENEMY_INVINCIBLE;
           break;
      case TANK:
           pSprite->pattern = (pEnemy->velocity > 0) ? PATTERN_RIGHT_TANK : PATTERN_LEFT_TANK;
           break;
*/
    }
  }
}

byte addBullet(byte aim, byte y)
{
   bulletData* pBullet = &bullets[aim][y / 3 - 1];
   if (!pBullet->inUse)
   {
      pBullet->inUse = TRUE;
      pBullet->posY = y;
      if (aim == MY_LEFT)
         pBullet->posX = 14;
      else
         pBullet->posX = 16;
      return TRUE;
   }
   return FALSE;
}

/* This is the main game loop */
void playGame()
{
   char vertMove;
   byte moved; /* do not allow player that moved horizontally to move vertically */

   vertMove = 0;
   allowMoveLeftMin = CENTERX;
   allowMoveRightMax = CENTERX;

   disable_nmi();

   enemyColor = 3;
   flip2 = 0;
   flip3 = 0;
   flip4 = 0;
   flip5 = 0;
   flip6 = 0;

   /*memset(bullets, 0x00, sizeof(bullets));
   memset(enemies, 0x00, sizeof(enemies));
   */
   for (i = 0; i<NUM_ROWS; i++) {
     enemies[i].inUse = ENEMY_NOT_IN_USE;
     for (j=0; j<=1; j++) {
       bullets[j][i].inUse = 0;
     }
   }

   pointBacklog = 0;
   levelLength = level[player] * 10 + 20;
   minEnemySpeed = (level[player] << 3) + 128;
   maxEnemiesAllowed = (level[player] >> 1) + INITIAL_ENEMY_COUNT;
   if (maxEnemiesAllowed > NUM_ROWS)
      maxEnemiesAllowed = NUM_ROWS;

   shipPosX = CENTERX;
   shipPosY = CENTERY;
   shipAim = MY_RIGHT;
   shipBulletState = LOADED;

   /* this triggers game state in NMI to display various things */
   gameStatus = PLAYING;
   drawScreen(player);

   /* set up the ship positioning */
   sprites[SHIP_SPRITE].colour = 0;
   sprites[SHIP_SPRITE].x = 120;
   /*sprites[SHIP_SPRITE].y = SET DURING DRAW_SHIP */;
   sprites[SHIP_SPRITE].pattern = 0;
   drawShip(TRUE);

   while(gameStatus == PLAYING)
   {
      moved = FALSE;
      /* get user input */
      joypad = GetPlayerPad(player);

      /* Check if moving Left/Right */
      if (!flip2 && !vertMove) {

         /* if this row is a death egg, allow moving left & right */
         pEnemy = &enemies[shipPosY / 3 - 1];
         if ((pEnemy->type == DEATH_EGG) && (pEnemy->inUse != ENEMY_NOT_IN_USE)) {
            allowMoveLeftMin = 0;
            allowMoveRightMax = 30;
         }

         /* Disallow movement if we've reached the center under duress */
         if ((shipPosX == CENTERX) && (pEnemy->type == BURGER)) {
            allowMoveLeftMin = CENTERX;
            allowMoveRightMax = CENTERX;
         }

         if (joypad & RIGHT) {
            /* allow moving to right if not traveling vertically and aimed right and
               either allowed to move right or already on the left */
            if (!vertMove && (shipAim == MY_RIGHT) && ((shipPosX < allowMoveRightMax) || (shipPosX < CENTERX)))
            {
               eraseHorizMovement(shipPosX, shipPosY);
               shipPosX++;
               sprites[SHIP_SPRITE].x += 8;
               moved = TRUE;
            }
            shipAim = MY_RIGHT;
         }
         else if (joypad & LEFT) {
            /* allow moving to left if not traveling vertically and aimed left and
               either allowed to move left or already on the right */
            if (!vertMove && (shipAim == MY_LEFT) && ((shipPosX > allowMoveLeftMin) || (shipPosX > CENTERX)))
            {
               eraseHorizMovement(shipPosX+1, shipPosY);
               shipPosX--;
               sprites[SHIP_SPRITE].x -= 8;
               moved = TRUE;
            }
            shipAim = MY_LEFT;
         }
      }
      /* Check if moving Up/Down */
      else if (flip2 && !moved) {
         /* Move top to bottom in chunks of 3 */
         if (shipPosX == CENTERX)
         {
            allowMoveLeftMin = CENTERX;
            allowMoveRightMax = CENTERX;

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

      updateEnemies();

      /* Allow shooting if we are on a line that allows shooting */
      if (shipBulletState == LOADED)
      {
         if (!(shipPosY % VERTSTEPS) && (shipPosX == CENTERX) && (GetPlayerPad(player) & FIRE1))
         {
            if (addBullet(shipAim, shipPosY))
               shipBulletState = SHOOTING;
         }
      } else {
         if (++shipBulletState > SHOOTING) shipBulletState = UNLOADED2;
      }

      /* Update the display */
      drawShip(FALSE);

      /* wait for the next frame */
      enable_nmi();
      delay(1);
      disable_nmi();
   }

   /* do blow up */
   if (gameStatus == DEAD) {
     
     /* don't forget to keep the score rolling if backlogged points */

     /* guy just died */
     guys[player]--;
   }
   
   while (pointBacklog > 0) {
     enable_nmi();
     delay(1);
     disable_nmi();
   }
}

/* This controls the game flipping between players, etc */
void gameLoop()
{
    player = PLAYER1;

    guys[PLAYER1] = NUM_GUYS;
    guys[PLAYER2] = (numPlayers > 1) ? NUM_GUYS : 0;
    score_reset(&scores[PLAYER1]);
    score_reset(&scores[PLAYER2]);
    score_reset(&scoresStored[PLAYER1]);
    score_reset(&scoresStored[PLAYER2]);

    level[PLAYER1] = level[PLAYER2] = 0;

    while ((guys[PLAYER1] > 0) || (guys[PLAYER2] > 0))
    {
       notifyPlayer();
       playGame();

       /* flip to the other guy */
       if (gameStatus == DEAD) {
         if (guys[1-player] != 0)
            player = 1-player;
       } else {
         level[player]++;
       }
    }
}

/* see if spr1 and spr2 have collided */
byte checkSprColl(byte spr1, byte spr2)
{
  return ((absdiff(sprites[spr1].x, sprites[spr2].x) < 14) &&
          (absdiff(sprites[spr1].y, sprites[spr2].y) < 5));
}

void main(void)
{
   nmiThird = 0;

   score_reset(&scores[HIGHSCORE]);

   gameStatus = DEAD;
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
