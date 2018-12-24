#include <coleco.h>
#include <getput1.h>

void nmi(void)
{
 update_sound();
}

void main(void)
{
 /* 'a' is a temporary variable */
 byte a;

 /* init screen */
 screen_mode_2_text();
 upload_default_ascii(BOLD);

 /* clear screen */
 cls();

 /* show menu */
 center_string(6,"MENU");
 print_at(10,9,"1. ENGLISH");
 print_at(10,11,"2. FRANCAIS");
 center_string(22,"************");

 a = choice_keypad_1(1,2);

 /* clear screen */
 cls();

 if (a==1)
 {
 /* show english message */
   print_at(3,3,"CONGRATULATION!");
   print_at(3,5,"YOU'VE JUST COMPILED A");
   center_string(11,"COLECOVISION PROJECT");
   center_string(22,"PRESS FIRE TO CONTINUE");
 }
 else
 {
 /* show english message */
   print_at(3,3,"FELICITATIONS!");
   print_at(3,5,"VOUS VENEZ DE COMPILER UN");
   center_string(11,"PROJET COLECOVISION");
   center_string(22,"PESER SUR FEU POUR CONTINUER");
 }

 /* play music1 */
 music1();

 /* wait for fire button */
 pause();
}