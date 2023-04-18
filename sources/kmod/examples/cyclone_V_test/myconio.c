#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <termios.h> /* tcgetattr(), tcsetattr() */
#include <unistd.h> /* read() */

#include <sys/select.h>
#include <sys/time.h> /* struct timeval, select() */

static uint8_t sInput[256];

static struct termios g_old_kbd_mode;

static void cooked(void)
{
	tcsetattr(0, TCSANOW, &g_old_kbd_mode);
}

static void raw(void)
{
	/**/
	struct termios new_kbd_mode;

	/* put keyboard (stdin, actually) in raw, unbuffered mode */
	tcgetattr(STDIN_FILENO, &new_kbd_mode);

	/* Already in raw mode, exit*/
	if((new_kbd_mode.c_lflag & (ICANON | ECHO)) == 0)
		return;

	/* Store original mode */
	memcpy(&g_old_kbd_mode, &new_kbd_mode, sizeof(struct termios));
	new_kbd_mode.c_lflag &= ~(ICANON | ECHO);
	new_kbd_mode.c_cc[VTIME] = 0;
	new_kbd_mode.c_cc[VMIN] = 1;
	tcsetattr(STDIN_FILENO, TCSANOW, &new_kbd_mode);
}

int kbhit(void)
{
	struct timeval timeout;
	fd_set read_handles;
	int status;

	raw();
	
	/* check stdin (fd 0) for activity */
	FD_ZERO(&read_handles);
	FD_SET(STDIN_FILENO, &read_handles);
	timeout.tv_sec = timeout.tv_usec = 0;
	status = select(STDIN_FILENO+1, &read_handles, NULL, NULL, &timeout);
	if(status < 0)
	{
		printf("select() failed in kbhit()\n");
		cooked();
		exit(1);
	}
	
	/* Restore original context, "cooked" mode */
	if(FD_ISSET(STDIN_FILENO, &read_handles))
	{
		cooked();
	}
	return status;
}

int getch(void)
{
	unsigned char temp;

	raw();

	/* stdin = fd 0 */
	if(read(STDIN_FILENO, &temp, 1) != 1)
	{
		temp = 0;
	}

	cooked();

	return temp;
}

void clrscr(void)
{
 printf("\033[2J");
}

int gotoxy(int x, int y) 
{
 printf("%c[%d;%df",0x1B,y,x); 
 return 0;
}

void fmygets(char * st, int szst)
{
  unsigned char c;
  int pz=0;
 memset(st, 0, szst);
 do
 {
  while(!kbhit());
  c=getch();
  switch(c)
  {
    case 8: if (pz!=0) 
            {
             printf("%c %c", 8, 8), fflush(stdout);
             st[--pz]=0;
            }
            break;
    case 0: getch();
            break;
   default: if ((c>31)||(c==10))
            if (pz<szst)
            {
             if (c!=10) st[pz++]=c;
             printf("%c", c), fflush(stdout);
            }
  }
 } while(c!=10);
// printf("\n%d\n", strlen(st));
}

uint32_t GetHexValue(const char * msg, uint32_t * value, uint32_t max)
{
	int iRet;
	
	printf("%s", msg), fflush(stdout);

//	msg=fgets((char*)sInput, sizeof(sInput), stdin);
	fmygets((char*)sInput, sizeof(sInput));

	iRet = sscanf((const char*)sInput, "%x \n", value);

	if(iRet < 1)
	{
		//printf("Invalid hex value!\r\n");
		return 1;
	}
	else if(max > 0 && *value > max)
	{
		printf("Value out of range!\r\n");
		return 2;
	}

	return 0;
}

void GetHexValue_with_correct(const char * msg, uint32_t * value, uint32_t max)
{
    do
    {
        if (GetHexValue(msg, value, max) == 0) break;
    } while(1);
}

uint32_t GetDecValue(const char * msg, uint32_t * value, uint32_t max)
{
	int iRet;

	printf("%s", msg), fflush(stdout);

	fmygets((char*)sInput, sizeof(sInput));
//	msg=fgets((char*)sInput, sizeof(sInput), stdin);
//   printf("\n%d\n", strlen(st));

//	printf("%s\n", msg);

	iRet = sscanf((const char*)sInput, "%d \n", value);

//	printf("%d\n", *value);

	if(iRet < 1)
	{
		//printf("Invalid decimal value!\r\n");
		return 1;
	}
	else if(max > 0 && *value > max)
	{
		printf("Value out of range!\r\n");
		return 2;
	}

	return 0;
}

void GetDecValue_with_correct(const char * msg, uint32_t * value, uint32_t max)
{
    do
    {
        if (GetDecValue(msg, value, max) == 0) break;
    } while(1);
}

