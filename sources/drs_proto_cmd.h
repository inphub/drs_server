/*
 * drs_proto_cmd.h
 *
 *  Created on: 25 October 2022
 *      Author: Dmitriy Gerasimov
 */
#pragma once
#include <dap_common.h>

#include <dap_events_socket.h>

typedef enum drs_proto_cmd{
    CMD_IDLE               = 0,

    CMD_REG_WRITE          = 1,   // write reg size 12
    CMD_REG_READ           = 2,   // read reg  size 8

    CMD_DATA_READ_DRS1     = 3,   // read data from drs1 size 16384
    CMD_DATA_READ_DRS2     = 103, // read data from drs1 size 16384

    CMD_SHIFT_READ_DRS1    = 4,   // read shift drs1
    CMD_SHIFT_READ_DRS2    = 104, // read shift drs2

    CMD_INI_READ           = 6,   // read ini
    CMD_INI_WRITE          = 7,   // write ini
    CMD_INI_FILE_READ      = 8,   // read file ini
    CMD_INI_FILE_WRITE     = 9,   // write file ini

    CMD_PAGE_READ_DRS1     = 0xA, //read N page data from DRS1
    CMD_PAGE_READ_DRS2     = 110, //read N page data from DRS1

    CMD_CALIBRATE          = 0xB, // Calibrate
    CMD_READ               = 0xC, // Read
    CMD_SHIFT_DAC_SET      = 0xD , // Set ShiftDAC
    CMD_FF                 = 0xE , // ??? Sends 15 always back
    CMD_GET_SHIFT          = 0xF, //получение массива сдвигов ячеек

    CMD_START              = 0x10, // старт
    CMD_READ_STATUS_N_PAGE = 0x12, //read status and page
    CMD_READ_PAGE          = 0x13, //read
} drs_proto_cmd_t;

extern size_t g_drs_proto_args_min_count[];

void drs_proto_cmd(dap_events_socket_t * a_es, drs_proto_cmd_t a_cmd, uint32_t* a_cmd_args, size_t a_cmd_args_count);

