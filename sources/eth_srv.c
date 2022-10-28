/*
 * eth_srv.c
 *
 *  Created on: 24 дек. 2014 г.
 *      Author: Zubarev
 *  Refactored on : 2022
 *      Author: Dmitriy Gerasimov <naeper@demlabs.net>
 */
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <signal.h>
#include <linux/fs.h>
#include <sys/socket.h>  /* sockaddr_in */
#include <netinet/in.h>  /* AF_INET, etc. */
#include <ctype.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "eth_srv.h"
#include "mem_ops.h"
#include "drs.h"
#include "drs_proto.h"

#include "minIni.h"
#include "calibrate.h"
#include "commands.h"
#include "data_operations.h"

#define DEBUG
#define SERVER_NAME "test_server"
#define PORT 3000
#define MAX_SLOWBLOCK_SIZE 1024*1024
#define SIZE_BUF_IN 128


#define SIZE_NET_PACKAGE 1024//0x100000 // 0x8000 = 32k
#define POLLDELAY 1000 //ns

#define MAX_SLOW_ADC_CHAN_SIZE 0x800000 
#define MAX_SLOW_ADC_SIZE_IN_BYTE MAX_SLOW_ADC_CHAN_SIZE*8*2



static unsigned short mas[0x4000];
static unsigned short tmas[MAX_SLOW_ADC_SIZE_IN_BYTE+8192*2];// + shift size
static unsigned short tmasFast[SIZE_FAST];
static unsigned char buf_in[SIZE_BUF_IN];
static float gainAndOfsetSlow[16];
static unsigned int *cmd_data=(uint32_t *)&buf_in[0];//(unsigned int *)
static struct sockaddr_in name, cname;
static struct timeval tv = { 0, POLLDELAY };

//int fp;

static int s, ns;
static long int nbyte=0;
static unsigned int clen;
static char disconnect;
static fd_set rfds;

static parameter_t ini={0};

/**
 * @brief eth_srv_init
 * @return
 */
int eth_srv_init(void)
{
    g_ini = &ini;
    //ZeroMemory
    memset(&ini, 0, sizeof(parameter_t));
    //	save_to_ini(inipath, &ini);
    drs_ini_load(g_inipath, &ini);
    printf("ROFS1=%u\n", ini.fastadc.ROFS1);
    printf("OFS1=%u\n", ini.fastadc.OFS1);
    printf("ROFS2=%u\n", ini.fastadc.ROFS2);
    printf("OFS2=%u\n", ini.fastadc.OFS2);
    printf("CLK_PHASE=%u\n", ini.fastadc.CLK_PHASE);

    //	test_data=(uint32_t *)malloc(SIZE_BUF_OUT);
    //	printf("server: memset "),fflush(stdout);
    //	memset(test_data, 1, SIZE_BUF_OUT);
    //	printf("ok!\n\r"),fflush(stdout);
    //	if (init_mem()!=0) { printf("init error"),fflush(stdout); return -1; }
    if (init_mem())
        return -1;

    memw(0xFFC25080,0x3fff); //инициализация работы с SDRAM
    write_reg(23, 0x8000000);// DRS1
    write_reg(25, 0x4000);
    write_reg(24, 0xC000000);// DRS2
    write_reg(26, 0x4000);
    write_reg(28, 0xBF40000);
    write_reg(29, 0xFF40000);


    drs_init(&ini);
    return 0;
}

/**
 * @brief eth_srv_deinit
 */
void eth_srv_deinit()
{
   //   free(test_data);
      deinit_mem();
   //   close(fp);
}


