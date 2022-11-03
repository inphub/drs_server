/*
 * drs_proto_cmd.c
 *
 *  Created on: 25 October 2022
 *      Author: Dmitriy Gerasimov
 */
#include <dap_common.h>

#include "drs.h"
#include "drs_proto.h"
#include "drs_proto_cmd.h"

#include "commands.h"
#include "calibrate.h"
#include "data_operations.h"

#define LOG_TAG "drs_proto_cmd"

size_t g_drs_proto_args_size[DRS_PROTO_CMD_MAX]={
    [CMD_REG_WRITE]           = 2 * sizeof(uint32_t),
    [CMD_REG_READ]            = 1 * sizeof(uint32_t),

    [CMD_PAGE_READ_DRS1]      = 1 * sizeof(uint32_t),
    [CMD_PAGE_READ_DRS2]      = 1 * sizeof(uint32_t),
    [CMD_INI_FILE_WRITE]      = sizeof(*g_ini),
    [CMD_INI_WRITE]           = sizeof(*g_ini),
    [CMD_CALIBRATE]           = 6 * sizeof(uint32_t),
    [CMD_READ]                = 3 * sizeof(uint32_t),
    [CMD_SHIFT_DAC_SET]       = 1 * sizeof(uint32_t),
    [CMD_FF]                  = 1 * sizeof(uint32_t),
    [CMD_GET_SHIFT]           = 1 * sizeof(uint32_t),
    [CMD_START]               = 2 * sizeof(uint32_t),
    [CMD_READ_PAGE]           = 3 * sizeof(uint32_t),
};

#define MAX_PAGE_COUNT 1000
//#define SIZE_FAST MAX_PAGE_COUNT*1024*8*8

coefficients_t COEFF = {};
uint32_t shift [DRS_COUNT * 1024] = {0};
double ddata[16384] = {0};


void drs_proto_cmd(dap_events_socket_t * a_es, drs_proto_cmd_t a_cmd, uint32_t* a_cmd_args)
{
    uint32_t l_addr = 0, l_value = 0;
    log_it(L_DEBUG, "Proto cmd 0x%08X", a_cmd);
    switch( a_cmd ) {
        case CMD_IDLE:
            log_it(L_NOTICE, "Client sent idle command, do nothing");
        break;

        case CMD_REG_WRITE: // Write reg size 12
            l_addr=a_cmd_args[0];
            l_value=a_cmd_args[1];
            write_reg(l_addr, l_value);
            log_it(L_DEBUG,"write: adr=0x%08x, val=0x%08x", l_addr, l_value);
            l_value=0;
            dap_events_socket_write_unsafe(a_es, &l_value, sizeof(l_value) );
        break;

        case CMD_REG_READ: //read reg  size 8
            l_addr=a_cmd_args[0];
            l_value=read_reg(l_addr);
            log_it( L_DEBUG,"read: adr=0x%08x, val=0x%08x", l_addr, l_value);
            dap_events_socket_write_unsafe(a_es, &l_value, sizeof(l_value) );
        break;

        case CMD_DATA_READ_DRS1: //read data from drs1 size 16384
            log_it(L_DEBUG, "read data DRS1: [0]=0x%04x,[1]=0x%04x [2]=0x%04x,[3]=0x%04x [4]=0x%04x,[5]=0x%04x [6]=0x%04x,[7]=0x%04x",
                   ((unsigned short *)data_map_drs1)[0], ((unsigned short *)data_map_drs1)[1], ((unsigned short *)data_map_drs1)[2], ((unsigned short *)data_map_drs1)[3], ((unsigned short *)data_map_drs1)[4], ((unsigned short *)data_map_drs1)[5], ((unsigned short *)data_map_drs1)[6], ((unsigned short *)data_map_drs1)[7]);
            drs_proto_out_add_mem( DRS_PROTO(a_es), data_map_drs1, DRS_DATA_MAP_SIZE);
        break;

        case CMD_DATA_READ_DRS2: //read data from drs2 size 16384
            log_it(L_DEBUG, "read data DRS2: [0]=0x%04x,[1]=0x%04x [2]=0x%04x,[3]=0x%04x [4]=0x%04x,[5]=0x%04x [6]=0x%04x,[7]=0x%04x", ((unsigned short *)data_map_drs2)[0], ((unsigned short *)data_map_drs2)[1], ((unsigned short *)data_map_drs2)[2], ((unsigned short *)data_map_drs2)[3], ((unsigned short *)data_map_drs2)[4], ((unsigned short *)data_map_drs2)[5], ((unsigned short *)data_map_drs2)[6], ((unsigned short *)data_map_drs2)[7]);
            drs_proto_out_add_mem( DRS_PROTO(a_es), data_map_drs2, DRS_DATA_MAP_SIZE);
        break;

        case CMD_SHIFT_READ_DRS1:{ //read shift drs1
            uint16_t l_tmpshift=((unsigned long *)data_map_shift_drs1)[0];
            log_it( L_DEBUG, "read shifts: shift DRS1 = 0x%04x, shift DRS1=0x%04x", ((unsigned short *)data_map_shift_drs1)[0], ((unsigned short *)data_map_shift_drs2)[0]);
            dap_events_socket_write_unsafe( a_es, &l_tmpshift, sizeof (l_tmpshift));
            break;}

        case CMD_SHIFT_READ_DRS2:{ //read shift drs2
            uint16_t l_tmpshift=((unsigned long *)data_map_shift_drs1)[0];
            log_it( L_DEBUG, "read shifts: shift DRS1 = 0x%04x, shift DRS2=0x%04x", ((unsigned short *)data_map_shift_drs1)[0], ((unsigned short *)data_map_shift_drs2)[0]);
            dap_events_socket_write_unsafe( a_es, &l_tmpshift, sizeof (l_tmpshift));
            break;}

        case CMD_INI_READ: //read ini
            assert (g_ini);
            log_it(L_DEBUG, "read ini, sizeof(g_ini)=%zd", sizeof(*g_ini));
            drs_proto_out_add_mem(DRS_PROTO(a_es),g_ini, sizeof(*g_ini));
        break;

        case CMD_INI_FILE_WRITE: //write file ini
        case CMD_INI_WRITE: //write ini
            log_it(L_DEBUG, "write ini (size %zd)", a_es->buf_in_size);
            memcpy(g_ini, a_cmd_args, sizeof(*g_ini) );
            l_value=0;
            dap_events_socket_write_unsafe ( a_es, &l_value, sizeof(l_value));
        break;

        case CMD_INI_FILE_READ: //read file ini
            log_it(L_DEBUG, "read ini file");
            int filefd;
            filefd = open(g_inipath, O_RDONLY);
            if (filefd == -1) {
                char l_errstr[255];
                int l_errno = errno;
                l_errstr[0] = '\0';
                strerror_r(l_errno, l_errstr, sizeof(l_errstr));
                log_it(L_ERROR, "Can't open \"%s\": %s (code %d)", g_inipath, l_errstr, l_errno);
                break;
            }
            drs_proto_out_add_file(DRS_PROTO(a_es), filefd, true);
        break;

        case CMD_PAGE_READ_DRS1:{//read N page data from DRS1
            log_it(L_DEBUG,"Read %u pages",a_cmd_args[0]);

            size_t t;
            for (t=0; t<a_cmd_args[0]; t++) {
                drs_proto_out_add_mem(DRS_PROTO(a_es),&(((unsigned short *)data_map_drs1)[t*DRS_PAGE_SIZE]), 0x4000 );
            }
            if (t>0) t--;
            log_it( L_DEBUG, "read page %d data: [0]=0x%04x,[1]=0x%04x [2]=0x%04x,[3]=0x%04x [4]=0x%04x,[5]=0x%04x [6]=0x%04x,[7]=0x%04x\n", t, ((unsigned short *)data_map_drs1)[t*8192+0], ((unsigned short *)data_map_drs1)[t*8192+1], ((unsigned short *)data_map_drs1)[t*8192+2], ((unsigned short *)data_map_drs1)[t*8192+3], ((unsigned short *)data_map_drs1)[t*8192+4], ((unsigned short *)data_map_drs1)[t*8192+5], ((unsigned short *)data_map_drs1)[t*8192+6], ((unsigned short *)data_map_drs1)[t*8192+7]);
            log_it( L_DEBUG, "read shift: %4lu\n", ((unsigned long *)data_map_shift_drs1)[0]);
        }break;

        case CMD_PAGE_READ_DRS2:{//read N page data from DRS2
            log_it(L_DEBUG,"Read %u pages",a_cmd_args[0]);

            size_t t;
            for (t=0; t<a_cmd_args[0]; t++) {
                drs_proto_out_add_mem(DRS_PROTO(a_es),&(((unsigned short *)data_map_drs1)[t*DRS_PAGE_SIZE]), 0x4000 );
            }
            if (t>0) t--;
            log_it( L_DEBUG, "read page %d data: [0]=0x%04x,[1]=0x%04x [2]=0x%04x,[3]=0x%04x [4]=0x%04x,[5]=0x%04x [6]=0x%04x,[7]=0x%04x\n", t, ((unsigned short *)data_map_drs1)[t*8192+0], ((unsigned short *)data_map_drs1)[t*8192+1], ((unsigned short *)data_map_drs1)[t*8192+2], ((unsigned short *)data_map_drs1)[t*8192+3], ((unsigned short *)data_map_drs1)[t*8192+4], ((unsigned short *)data_map_drs1)[t*8192+5], ((unsigned short *)data_map_drs1)[t*8192+6], ((unsigned short *)data_map_drs1)[t*8192+7]);
            log_it( L_DEBUG, "read shift: %4lu\n", ((unsigned long *)data_map_shift_drs1)[0]);
        }break;

        case CMD_CALIBRATE:{         /*
            calibrate

            a_cmd_args[0] - ключи калибровки, 1 бит амплитудная,2 локальная временная,3 глобальная временная
            a_cmd_args[1] - N c клиента, количество проходов аплитудной калибровки для каждого уровня цапов
            a_cmd_args[2] - Min N с клиента, минимальное число набора статистики для каждой ячейки в локальной калибровке
            a_cmd_args[3] - numCylce, число проходов в глобальной колибровке
            a_cmd_args[4] - количество уровней у амплитудной калибровки count,
            для каждого будет N(из a_cmd_args[2]) проходов,
            при нуле будут выполняться два прохода для уровней BegServ и EndServ(о них ниже),
            при не нулевом значении, между  BegServ и EndServ будут включены count дополнительных уровней цапов для амплитудной калибровки
            с a_cmd_args[5] идет массив даблов, первые 2 элемента BegServ и EndServ
            остальные 8 сдвиги цапов
            */

            double * calibLvl=(double *)(&a_cmd_args[5]);
            double * shiftDAC=&calibLvl[2];
            size_t t;
            l_value=0;

            for(t=0;t<6;t++){
                log_it(L_DEBUG, "cmd_arg[%zd] = %u", t, a_cmd_args[t]);
            }

            for(t=0;t<2;t++){
                log_it(L_DEBUG, "calibLvl[%zd] = %f", t, calibLvl[t]);
            }

            for(t=0;t<8;t++){
                log_it(L_DEBUG, "shiftDAC[%zd] = %f", t, shiftDAC[t]);
            }
            setNumPages(1);
            //setSizeSamples(1024);//Peter fix
            if((a_cmd_args[0]&1)==1){
                log_it(L_DEBUG, "start amplitude calibrate\n");
                l_value=calibrate_amplitude(&COEFF,calibLvl,a_cmd_args[1],shiftDAC,g_ini,a_cmd_args[4]+2)&1;
                if(l_value==1){
                    log_it(L_DEBUG, "end amplitude calibrate");
                }else{
                    log_it(L_DEBUG, "amplitude calibrate error");
                }
            }
            if((a_cmd_args[0]&2)==2){
                log_it(L_DEBUG, "start time calibrate");
                l_value|=(calibrate_time(a_cmd_args[2],&COEFF,g_ini)<<1)&2;
                if((l_value&2)==2){
                    log_it(L_DEBUG, "end time calibrate");
                }else{
                    log_it(L_DEBUG, "time calibrate error");
                }
            }
            if((a_cmd_args[0]&4)==4){
                log_it(L_DEBUG, "start global time calibrate");
                l_value|=(calibrate_time_global(a_cmd_args[3],&COEFF,g_ini)<<2)&4;
                if((l_value&4)==4){
                    log_it(L_DEBUG, "end global time calibrate");
                }else{
                    log_it(L_DEBUG, "global time calibrate error");
                }
            }
            dap_events_socket_write_unsafe(a_es, &l_value, sizeof(l_value));
        }break;

        case CMD_READ:
            /*
            read
            a_cmd_args[0]- число страниц для чтения
            a_cmd_args[1]- флаг для soft start
            a_cmd_args[2]- флаг для передачи массивов X
            */
            l_value=4*(1+((a_cmd_args[2]&24)!=0));
            log_it(L_DEBUG, "Npage %u",a_cmd_args[0]);
            log_it(L_DEBUG, "soft start %u",a_cmd_args[1]);
            if((a_cmd_args[1]&1)==1){//soft start
                setNumPages(1);
                //setSizeSamples(1024);//Peter fix
                onceGet(&tmasFast[0],&shift[0],0,0,0);
                //onceGet(&tmasFast[8192],&shift[1024],0,0,1);
            }else{
                readNPages(&tmasFast[0],shift,a_cmd_args[0], l_value*16384, 0);//8192
                //readNPages(&tmasFast[??],shift,a_cmd_args[0], val*16384, 1);//8192
            }
            for(size_t t=0;t<a_cmd_args[0];t++) {
                calibrate_do_curgr(&tmasFast[t*8192*l_value],ddata,&shift[t],&COEFF,1024,8,a_cmd_args[2],g_ini);
                log_it(L_DEBUG, "tmasFast[%d]=%f shift[%d]=%d",t*8192,ddata[0],t,shift[t]);
                memcpy(&tmasFast[t*8192*l_value],ddata,sizeof(double)*8192);
                if(l_value==8){
                    calibrate_get_array_x(ddata,&shift[t],&COEFF,a_cmd_args[2]);
                    memcpy(&tmasFast[(t*l_value+4)*8192],ddata,sizeof(double)*8192);
                }
            }
            log_it(L_DEBUG, "buferSize=%u",2*8192*a_cmd_args[0]*l_value);
            drs_proto_out_add_mem(DRS_PROTO(a_es), tmasFast,2*8192*a_cmd_args[0]*l_value);
        break;

        case CMD_SHIFT_DAC_SET:{
            //set shift DAC
            double *shiftDAC=(double *)(&a_cmd_args[0]);
            for(size_t t=0;t<4;t++){
            log_it(L_DEBUG, "%f",shiftDAC[t]);
            }
            setShiftAllDac(shiftDAC,g_ini->fastadc.dac_gains, g_ini->fastadc.dac_offsets);
            setDAC(1);
            l_value=1;
            dap_events_socket_write_unsafe(a_es, &l_value, sizeof(l_value));
        }break;

        case CMD_FF:
            log_it(L_DEBUG,"ff %d",a_cmd_args[0]);
            l_value=15;
            dap_events_socket_write_unsafe(a_es, &l_value, sizeof(l_value));
        break;

        case CMD_GET_SHIFT:
            //получение массива сдвигов ячеек
            //memcpy(tmasFast,shift,sizeof(unsigned int)*a_cmd_args[0]);
            dap_events_socket_write_unsafe(a_es, &shift, sizeof(unsigned int)*a_cmd_args[0]);
        break;

        case CMD_START:
            //старт
            //a_cmd_args[0]- mode
            //		00 - page mode and read (N page)
            //		01 - soft start and read (one page)
            //		02 - only read
            //		04 - external start and read (one page)
            //a_cmd_args[1] - pages
            switch(a_cmd_args[0]){
                case 0: //00 - page mode and read (N page)
                    setNumPages(a_cmd_args[1]);
                    log_it(L_DEBUG, "write: adr=0x%08x, val=0x%08x\n", 0x00000025, a_cmd_args[1]);
                    setSizeSamples(a_cmd_args[1]*1024);
                    log_it(L_DEBUG, "write: adr=0x%08x, val=0x%08x\n", 0x00000019, a_cmd_args[1]*1024);
                    write_reg(0x00000015, 1<<1);  //page mode
                    log_it(L_DEBUG, "write: adr=0x%08x, val=0x%08x\n", 0x00000015, 1<<1);
                    usleep(100);
                    write_reg(0x00000027, 1);  //external enable
                    log_it(L_DEBUG, "write: adr=0x%08x, val=0x%08x\n", 0x00000027, 1);
                break;
                case 1: //01 - soft start and read (one page)
                    log_it(L_DEBUG, "01 - soft start and read (one page)\n");
                    write_reg(0x00000015, 0);  //page mode disable
                    write_reg(0x00000027, 0);  //external disable
                    usleep(100);
                    /*                                       	 write_reg(0x00000025, 1);   	//pages
                    log_it(L_DEBUG, "write: adr=0x%08x, val=0x%08x\n", 0x00000025, 1);
                    write_reg(0x00000019, 1024);  //size samples
                    log_it(L_DEBUG, "write: adr=0x%08x, val=0x%08x\n", 0x00000019, 1024);
                    //write_reg(0x00000015, 1<<1);  //page mode
                    usleep(100);
                    write_reg(0x00000010, 1);  //soft start
                    log_it(L_DEBUG, "write: adr=0x%08x, val=0x%08x\n", 0x00000010, 1);
                    */
                break;
                case 2: //02 - only read
                break;

                case 4: //04 - external start and read (one page)
                    printf("04 - external start and read");
                    setNumPages(1);
                    printf("write: adr=0x%08x, val=0x%08x\n", 0x00000025, 1), fflush(stdout);
                    setSizeSamples(1024);
                    printf("write: adr=0x%08x, val=0x%08x\n", 0x00000019, 1024), fflush(stdout);
                    write_reg(0x00000015, 1<<1);  //page mode
                    printf("write: adr=0x%08x, val=0x%08x\n", 0x00000015, 1<<1), fflush(stdout);
                    usleep(100);
                    write_reg(0x00000027, 1);  //external enable
                    printf("write: adr=0x%08x, val=0x%08x\n", 0x00000027, 1), fflush(stdout);
                break;

                default:
                break;
            }
            l_value=0;
            dap_events_socket_write_unsafe(a_es, &l_value, sizeof(l_value));
        break;
        case CMD_READ_STATUS_N_PAGE:{ //read status and page
            //нужно подправить!!!
            struct fast_compile {
                uint32_t fast_complite;
                uint32_t pages;
            } DAP_ALIGN_PACKED l_reply = {
                .fast_complite = read_reg(0x00000031),//fast complite
                .pages = read_reg(0x0000003D),//num of page complite
            };
            //printf("fast complite: %d, num page complite: %d, slow complite %d\n", a_cmd_args[0], a_cmd_args[1], a_cmd_args[2]),fflush(stdout);
            dap_events_socket_write_unsafe(a_es, &l_reply, sizeof(l_reply));
        }break;
        case CMD_READ_PAGE:
            /*
            read
            a_cmd_args[0]- число страниц для чтения
            a_cmd_args[1]- флаг для soft start
            a_cmd_args[2]- флаг для передачи массивов X
            */
            l_value=4*(1+((a_cmd_args[2]&24)!=0));
            printf("Npage %u\n",a_cmd_args[0]);
            printf("soft start %u\n",a_cmd_args[1]);
            if((a_cmd_args[1]&1)==1){//soft start
                //                                		 readNPage(tmasFast,shift,0,1);
                setNumPages(1);
                setSizeSamples(1024);
                onceGet(&tmasFast[0],&shift[0],0,0,0);
                //                                		 onceGet(&tmasFast[8192],&shift[1024],0,0,1);
            }else{
                readNPages(&tmasFast[0],shift,a_cmd_args[0], l_value*16384, 0);//8192
                //                                		 readNPages(&tmasFast[??],shift,a_cmd_args[0], val*16384, 1);//8192
            }
            for(size_t t=0;t<a_cmd_args[0];t++){
                calibrate_do_curgr(&tmasFast[t*8192*l_value],ddata,shift,&COEFF,1024,4,a_cmd_args[2],g_ini);
                log_it(L_DEBUG, "tmasFast[%d]=%f shift[%d]=%d",t*8192,ddata[0],t,shift[t]);
                memcpy(&tmasFast[t*8192*l_value],ddata,sizeof(double)*8192);
                if(l_value==8){
                    calibrate_get_array_x(ddata,shift,&COEFF,a_cmd_args[2]);
                    memcpy(&tmasFast[(t*l_value+4)*8192],ddata,sizeof(double)*8192);
                }
            }
            log_it(L_DEBUG, "buferSize=%u",2*8192*a_cmd_args[0]*l_value);
            drs_proto_out_add_mem(DRS_PROTO(a_es), tmasFast,2*8192*a_cmd_args[0]*l_value);
        break;
        default: log_it(L_WARNING, "Unknown command %d", a_cmd);
    }
}


