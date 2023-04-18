#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h> /* read() */

#include <sys/select.h>
#include <sys/time.h> /* struct timeval, select() */

#include <cyclone_v_lib.h>
#include "myconio.h"

uint32_t adcdata[8][128*1024];

int fp;

unsigned char dont;

int main(void)
{
 unsigned char c;
 uint32_t value32;
 uint32_t number;

 fp = open("/dev/Cyclone_V_dev", O_RDWR);
 if(fp < 0)
 {
  printf("can't open device\n");
  return -1;
 }

 while(1)
 {
  if (dont!=1) printf("> "), fflush(stdout);
  dont=0;
  while(!kbhit());
  c=getch();
  switch(c)
  {
	case 'r': 
    case 'R': printf("Read regiter\n");
	          GetDecValue_with_correct("Enter register(0...63): ", &number, 63);
              cyclone_V_regread(fp, number, &value32);
              printf("Read register %u = %u(0x%08X)\n", number, value32, value32);
	          break;
	case 'w': 
    case 'W': printf("Write register\n");
	          GetDecValue_with_correct("Enter register(0...63): ", &number, 63);
              GetHexValue_with_correct("Enter hex value(0...FFFFFFFF): ", &value32, 0xFFFFFFFF);// 4294967295
              printf("Write register %u value %u(0x%08X)\n", number, value32, value32);
              cyclone_V_regwrite(fp, number, value32);
	          break;
	case '0': close(fp);
              printf("\nExit\n");
		      exit(0);
	default: dont=1;
  }
  if (dont<1) printf("-------------------------------------------------------------\n");
 }
 return 0;
}

