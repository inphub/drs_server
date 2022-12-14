/*
 * drs_proto_cmd.h
 *
 *  Created on: 1 November 2022
 *      Author: Dmitriy Gerasimov <dmitry.gerasimov@demlabs.net>
 */

#include <dap_common.h>
#include <dap_string.h>
#include <dap_strfuncs.h>
#include <dap_cli_server.h>

#include "drs.h"
#include "drs_ops.h"
#include "drs_cli.h"
#include "drs_data.h"
#include "drs_cal.h"

#define LOG_TAG "drs_cli"

static int s_callback_init(int a_argc, char ** a_argv, char **a_str_reply);
static int s_callback_start(int a_argc, char ** a_argv, char **a_str_reply);
static int s_callback_read(int a_argc, char ** a_argv, char **a_str_reply);
static int s_callback_calibrate(int argc, char ** argv, char **str_reply);
static int s_callback_help(int argc, char ** argv, char **str_reply);
static int s_callback_exit(int argc, char ** argv, char **str_reply);

static void s_calibrate_state_print(dap_string_t * a_reply, drs_calibrate_t *a_state,unsigned a_limits, int a_flags );
static int s_parse_drs_and_check(int a_arg_index, int a_argc, char ** a_argv, char **a_str_reply);


/**
 * @brief drs_cli_init
 * @return
 */
int drs_cli_init()
{
    // Help
    dap_cli_server_cmd_add ("help", s_callback_help, "Description of command parameters",
                                        "help [<command>]\n"
                                        "\tObtain help for <command> or get the total list of the commands\n"
                                        );
    // Exit
    dap_cli_server_cmd_add ("exit", s_callback_exit, "Stop application and exit",
                "exit\n" );

    // Init DRS
    dap_cli_server_cmd_add ("init", s_callback_init, "Init DRS",
                "init\n"
                "\tInit DRS\n"
             );


    // Start DRS
    dap_cli_server_cmd_add ("start", s_callback_start, "Start DRS",
                "start [<DRS number>]\n"
                "\tStart all DRS or just the <DRS number> if present\n"
             );

    // Calibrate
    dap_cli_server_cmd_add ("calibrate", s_callback_calibrate, "Calibrate DRS",
                "\n"
                "calibrate run [-drs <DRS number>]  -flags <AMPL,TIME_GLOBAL,TIME_LOCAL> [-repeats <repeats for Ampl>\n"
                "               -begin <Begin level> -end <End level> -shifts <Shifts for every DCA, splitted with \",\">\n"
                "               -N <Number of levels for ampl cal>] [-num_cycle <Cycles number for time global>]\n "
                "               [-min_N <Minimal N for time local calibration>]\n"
                "\tRun calibration process for specified DRS number or for all channels if number is not specified\n"
                "\n"
                "calibrate state [-drs <DRS number>] [-coeffs <SPLASH,DELTA_TIME,CHAN_K,CHAN_B,K_TIME,K,B,K9,B9>| ALL] [-limit <Array print limits>]\n"
                "\tCheck calibration status for specified DRS number or for all channels if number is not specified\n"
                ""
                "\n"
                "calibrate abort [-drs <DRS number>]\n"
                "\tAbort calibration specified DRS number or for all channels if number is not specified\n"
                "\n"
                           );

    // Get raw data
    dap_cli_server_cmd_add ("read", s_callback_read, "Read raw data ",
                            "\n"
                            "read write_ready [-drs <DRS num>]"
                            "\t Check for write_ready flag for target DRS or for all"
                            "\n"
                            "read page [-drs <DRS num>]"
                            "\t Call getOnce() and read one raw page at once for target DRS or for all"
                            "\n"
                            ""
                            "\n"
                            ""
                            );

    return 0;
}

/**
 * @brief drs_cli_deinit
 */
void drs_cli_deinit()
{

}

/**
 * @brief s_parse_drs_and_check
 * @param a_arg_index
 * @param a_argc
 * @param a_argv
 * @param a_str_reply
 * @return
 */
static int s_parse_drs_and_check(int a_arg_index, int a_argc, char ** a_argv, char **a_str_reply)
{
    const char * l_arg_drs = NULL;
    dap_cli_server_cmd_find_option_val(a_argv,a_arg_index, a_argc, "-drs", &l_arg_drs);

    if (l_arg_drs){
        int l_drs_num = atoi( l_arg_drs);
        // ????????? ???????????? ?????
        if (l_drs_num <0 || l_drs_num >= DRS_COUNT){
            dap_cli_server_cmd_set_reply_text(a_str_reply, "Wrong drs num %u, shoudn't be more than 0 and less than %u",
                                              l_drs_num, DRS_COUNT);
            return -100;
        }
        return l_drs_num;
    }
    return -1;
}

/**
 * @brief s_callback_init
 * @param a_argc
 * @param a_argv
 * @param a_str_reply
 * @return
 */
static int s_callback_init(int a_argc, char ** a_argv, char **a_str_reply)
{
    // ????????????? DRS
    if(drs_cmd_init(NULL) != 0){
        log_it(L_CRITICAL, "Can't init drs protocol");
        return -13;
    }
    dap_cli_server_cmd_set_reply_text(a_str_reply, "DRS initialized");
    return 0;
}

/**
 * @brief s_callback_start
 * @param a_argc
 * @param a_argv
 * @param a_str_reply
 * @return
 */
static int s_callback_start(int a_argc, char ** a_argv, char **a_str_reply)
{
    int l_arg_index = 0;

    int l_drs_num = s_parse_drs_and_check(l_arg_index, a_argc, a_argv, a_str_reply);
    if (l_drs_num < -1 ) // Wrong DRS num
        return -1;

    double l_shifts[DRS_DAC_COUNT]={30000,30000};
    float l_gains[DRS_DAC_COUNT]= {g_ini->fastadc.dac_gains[l_drs_num*DRS_COUNT],
                                   g_ini->fastadc.dac_gains[l_drs_num*DRS_COUNT + 1]};
    float l_offsets[DRS_DAC_COUNT]={g_ini->fastadc.dac_offsets[l_drs_num*DRS_COUNT],
                                    g_ini->fastadc.dac_offsets[l_drs_num*DRS_COUNT + 1]};

    drs_dac_shift_set_all(l_drs_num, l_shifts,l_gains, l_offsets );
    drs_start(l_drs_num);

    if (l_drs_num == -1)
        dap_cli_server_cmd_set_reply_text(a_str_reply,"DRS started all" );
    else
        dap_cli_server_cmd_set_reply_text(a_str_reply,"DRS started %d", l_drs_num );
    return 0;
}

/**
 * @brief s_callback_data
 * @param argc
 * @param argv
 * @param str_reply
 * @return
 */
static int s_callback_read(int a_argc, char ** a_argv, char **a_str_reply)
{
    enum {
        CMD_NONE =0,
        CMD_WRITE_READY,
        CMD_PAGE
    };
    int l_arg_index = 1;
    int l_cmd_num = CMD_NONE;

    // Get command
    if( dap_cli_server_cmd_find_option_val(a_argv, l_arg_index, a_argc, "write_ready", NULL)) {
        l_cmd_num = CMD_WRITE_READY;
    }else if( dap_cli_server_cmd_find_option_val(a_argv, l_arg_index, a_argc, "page", NULL)) {
        l_cmd_num = CMD_PAGE;
    }

    l_arg_index++;

    // ???????? ????? DRS
    int l_drs_num = s_parse_drs_and_check(l_arg_index, a_argc, a_argv, a_str_reply);
    if (l_drs_num < -1 ) // Wrong DRS num
        return -1;

    switch (l_cmd_num){
        case CMD_WRITE_READY:{
                bool l_flag_ready=drs_get_flag_write_ready(l_drs_num);
                dap_cli_server_cmd_set_reply_text( a_str_reply, "DRS #%d is %s", l_flag_ready? "ready": "not ready");
        }break;
        case CMD_PAGE:{
            const char * l_limits_str;
            size_t l_limits = DRS_CELLS_COUNT;
            dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-limit",  &l_limits_str);

            if (l_limits_str)
                l_limits = atoi(l_limits_str);

            if(l_drs_num!=-1){
                memset(tmasFast, 0, sizeof(tmasFast));
                dap_string_t * l_reply = dap_string_new("");
                dap_string_append_printf(l_reply,"Page read for DRS %d\n", l_drs_num);
                drs_t * l_drs = &g_drs[l_drs_num];
                drs_data_get_all(l_drs, 0, tmasFast);
                for (size_t t = 0; t < l_limits; t++){
                    dap_string_append_printf(l_reply, "0x%04X ", tmasFast[t]);
                    if ( t % 30 == 0)
                        dap_string_append_printf(l_reply, "\n");
                }
                dap_string_append_printf(l_reply, "\n");
                *a_str_reply = dap_string_free(l_reply, false);
            }else{
                dap_string_t * l_reply = dap_string_new("");

                for(size_t n = 0; n < DRS_COUNT ; n++){
                    memset(tmasFast, 0, sizeof(tmasFast));
                    dap_string_append_printf(l_reply,"Page read for DRS %d\n", n);
                    drs_t * l_drs = g_drs+ n;
                    drs_data_get_all(l_drs, 0, tmasFast);
                    for (size_t t = 0; t < l_limits; t++){
                        dap_string_append_printf(l_reply, "%04X ", tmasFast[t]);
                        if ( t % 30 == 0)
                            dap_string_append_printf(l_reply, "\n");
                    }
                    dap_string_append_printf(l_reply, "\n");
                }
                *a_str_reply = dap_string_free(l_reply, false);
            }
        }break;
        default:
            dap_cli_server_cmd_set_reply_text( a_str_reply, "Wrong call, check help for this command");
            return -3;
    }
    return 0;
}

/**
 * @brief s_callback_calibrate
 * @param argc
 * @param argv
 * @param str_reply
 * @return
 */
static int s_callback_calibrate(int a_argc, char ** a_argv, char **a_str_reply)
{
    if(a_argc > 1) {
        int l_arg_index = 1;
        // ?????? ??? ???????? (???? ????)
        int l_drs_num = s_parse_drs_and_check(l_arg_index,a_argc,a_argv,a_str_reply) ; // -1 ?????? ??? ????
        if (l_drs_num < -1)
            return l_drs_num;

        if (strcmp(a_argv[1], "run") == 0 ){ // Subcommand "run"

            // ?????? ????????? ? ???????
            const char * l_flags_str = NULL;

            dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-flags",        &l_flags_str);

            // ????????? ??????? ??????
            if ( ! (l_flags_str)  ){
                dap_cli_server_cmd_set_reply_text(a_str_reply, "Flags arguments is missed, check help for the command");
                return -2;
            }

            // ????????????? ????????? ? ????????? ?? ????????????

            // ???????????? ?????
            char ** l_flags_strs = dap_strsplit(l_flags_str, ",",3);
            if (l_flags_strs == NULL){
                dap_cli_server_cmd_set_reply_text(a_str_reply, "Flags argument is empty");
                return -21;
            }
            uint32_t l_flags = 0;
            unsigned l_min_N = 0;
            double l_begin = 0;
            double l_end = 0;
            double l_shifts[DRS_DCA_COUNT_ALL] ={};
            unsigned l_repeats = 0;
            unsigned l_num_cycle = 0;
            unsigned l_N = 0;

            for (size_t i = 0; l_flags_strs[i]; i ++){
                // ?????????????? ?????
                if ( dap_strcmp(l_flags_strs[i], "AMPL") == 0)
                    l_flags |= DRS_CAL_FLAG_AMPL;
                if ( dap_strcmp(l_flags_strs[i], "TIME_LOCAL") == 0)
                    l_flags |= DRS_CAL_FLAG_TIME_LOCAL;
                if ( dap_strcmp(l_flags_strs[i], "TIME_GLOBAL") == 0)
                    l_flags |= DRS_CAL_FLAG_TIME_GLOBAL;
            }

            // ??????????? ??????????
            if (l_flags & DRS_CAL_FLAG_AMPL){
                const char * l_repeats_str = NULL;
                const char * l_N_str = NULL;
                const char * l_begin_str = NULL;
                const char * l_end_str = NULL;
                const char * l_shifts_str = NULL;
                dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-repeats",      &l_repeats_str);
                dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-begin",        &l_begin_str);
                dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-end",          &l_end_str);
                dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-shifts",       &l_shifts_str);
                dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-N", &l_N_str);

                // ????????? ??????? ???? ??????????
                if ( ! (l_repeats_str && l_N_str && l_begin_str && l_end_str && l_shifts_str && l_N_str)  ){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "Amplitude arguments is missed, check help for the command");
                    return -2;
                }

                // ???????????? ????????
                char ** l_shifts_strs = dap_strsplit(l_shifts_str, ",",DRS_CHANNELS_COUNT);
                if (l_shifts_strs == NULL){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "Shifts argument is empty");
                    return -22;
                }
                size_t l_shifts_num =0;
                for ( l_shifts_num = 0; l_shifts_strs[l_shifts_num]; l_shifts_num ++){
                    char * l_shift_str = l_shifts_strs[l_shifts_num];
                    char * l_shift_str_endptr = NULL;
                    double l_shift = strtod( l_shift_str, & l_shift_str_endptr);
                    if (l_shift_str_endptr == l_shift_str){
                        dap_cli_server_cmd_set_reply_text(a_str_reply, "Shift #%i can't be converted to double value (\"%s\"",
                                                          l_shifts_num,l_shift_str);
                        return -23;
                    }
                    l_shifts[l_shifts_num] = l_shift;
                }
                if (l_shifts_num < DRS_CHANNELS_COUNT){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "Shifts number %u is too small, should be %u",
                                                      l_shifts_num,DRS_CHANNELS_COUNT);
                    return -24;
                }
                if (l_shifts_num > DRS_CHANNELS_COUNT){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "Shifts number %u is too big, should be %u",
                                                      l_shifts_num,DRS_CHANNELS_COUNT);
                    return -25;
                }

                // ???????????? begin
                char * l_begin_str_endptr = NULL;
                l_begin = strtod( l_begin_str, & l_begin_str_endptr);
                if (l_begin_str_endptr == l_begin_str){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "Begin value \"%s\" can't be converted to double", l_begin_str );
                    return -26;
                }

                // ???????????? end
                char * l_end_str_endptr = NULL;
                l_end = strtod( l_end_str, & l_end_str_endptr);
                if (l_end_str_endptr == l_end_str){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "End value \"%s\" can't be converted to double", l_end_str );
                    return -27;
                }

                // ???????????? repeats
                char * l_repeats_str_endptr = NULL;
                l_repeats = strtoul( l_repeats_str, & l_repeats_str_endptr, 10);
                if (l_repeats_str_endptr == l_repeats_str){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "Repeats value \"%s\" can't be converted to unsigned integer", l_repeats_str );
                    return -28;
                }

                // ???????????? N
                char * l_N_str_endptr = NULL;
                l_N = strtoul( l_N_str, & l_N_str_endptr, 10);
                if (l_N_str_endptr == l_N_str){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "levels_count value \"%s\" can't be converted to unsigned integer", l_N_str );
                    return -31;
                }
            }

            // ????????? ????????? ??????????
            if (l_flags & DRS_CAL_FLAG_TIME_LOCAL){
                // ???????????? min_N
                const char * l_min_N_str = NULL;
                char * l_min_N_str_endptr = NULL;
                dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-min_N",        &l_min_N_str);
                l_min_N = strtoul( l_min_N_str, & l_min_N_str_endptr, 10);
                if (l_min_N_str_endptr == l_min_N_str){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "MinN value \"%s\" can't be converted to unsigned integer", l_min_N_str );
                    return -29;
                }
            }
            // ?????????? ????????? ??????????
            if (l_flags & DRS_CAL_FLAG_TIME_GLOBAL){
                const char * l_num_cycle_str = NULL;
                dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-num_cycle",    &l_num_cycle_str);
                // ???????????? num_cycle
                char * l_num_cycle_str_endptr = NULL;
                l_num_cycle = strtoul( l_num_cycle_str, & l_num_cycle_str_endptr, 10);
                if (l_num_cycle_str_endptr == l_num_cycle_str){
                    dap_cli_server_cmd_set_reply_text(a_str_reply, "num_cycle value \"%s\" can't be converted to unsigned integer", l_num_cycle_str );
                    return -30;
                }
            }

            // ?????????????? ????????? ??????????
            drs_calibrate_params_t l_params = {
                .ampl = {
                    .repeats = l_repeats,
                    .N = l_N,
                },
                .time_global = {
                    .num_cycle = l_num_cycle
                },
                .time_local = {
                    .min_N = l_min_N
                }
            };
            l_params.ampl.levels[0] = l_begin;
            l_params.ampl.levels[1] = l_end;
            memcpy(l_params.ampl.levels+2, l_shifts,sizeof (l_params.ampl.levels)-2 *sizeof(double) );

            dap_string_t * l_reply = dap_string_new("");

            int l_ret;
            if (l_drs_num == -1) // ???? ?? ?????? DRS ?????, ?? ??????? ???
                for (size_t i = 0; i < DRS_COUNT; i++){
                    l_ret = drs_calibrate_run(i, l_flags, &l_params);
                    if (l_ret == 0)
                        dap_string_append_printf( l_reply, "DRS #u calibration started\n");
                    else
                        dap_string_append_printf( l_reply, "DRS #u calibration start error, code %d\n", l_ret);
                }
            else{ // ???? ??????, ?? ?????? ??????????
                l_ret = drs_calibrate_run(l_drs_num, l_flags, &l_params);
                if (l_ret == 0)
                    dap_string_append_printf( l_reply, "DRS #u calibration started\n");
                else
                    dap_string_append_printf( l_reply, "DRS #u calibration start error, code %d\n", l_ret);
            }

            *a_str_reply = dap_string_free(l_reply, false);
        }else if  (strcmp(a_argv[1], "state") == 0 ){ // Subcommand "check"
            dap_string_t * l_reply = dap_string_new("Check drs calibration progress:\n");
            const char * l_coeffs_str = NULL;
            const char * l_limits_str = NULL;
            int l_coeffs_flags = 0;
            unsigned l_limits = 0;
            dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-coeffs",  &l_coeffs_str);
            dap_cli_server_cmd_find_option_val(a_argv,l_arg_index, a_argc, "-limits",  &l_limits_str);

            if (l_limits_str)
              l_limits = atoi(l_limits_str);

            if ( l_coeffs_str == NULL || dap_strcmp( l_coeffs_str, "ALL") == 0){
                log_it(L_DEBUG, "Coeffs: ALL");
                l_coeffs_flags = DRS_COEF_SPLASH |  DRS_COEF_DELTA_TIME | DRS_COEF_CHAN_K | DRS_COEF_CHAN_B |
                                 DRS_COEF_K_TIME | DRS_COEF_K | DRS_COEF_B | DRS_COEF_K9 | DRS_COEF_B9;
            }else{
                log_it(L_DEBUG, "Coeffs: %s",l_coeffs_str);
                char ** l_coeffs_strs = dap_strsplit(l_coeffs_str, ",",10);
                if(l_coeffs_strs){
                    for (size_t i = 0; l_coeffs_strs[i]; i ++){
                        if( dap_strcmp(l_coeffs_strs[i], "SPLASH") == 0 )
                            l_coeffs_flags |= DRS_COEF_SPLASH;
                        else if( dap_strcmp(l_coeffs_strs[i], "DELTA_TIME") == 0 )
                            l_coeffs_flags |= DRS_COEF_DELTA_TIME;
                        else if( dap_strcmp(l_coeffs_strs[i], "CHAN_K") == 0 )
                            l_coeffs_flags |= DRS_COEF_CHAN_K;
                        else if( dap_strcmp(l_coeffs_strs[i], "CHAN_B") == 0 )
                            l_coeffs_flags |= DRS_COEF_CHAN_B;
                        else if( dap_strcmp(l_coeffs_strs[i], "K_TIME") == 0 )
                            l_coeffs_flags |= DRS_COEF_K_TIME;
                        else if( dap_strcmp(l_coeffs_strs[i], "K") == 0 )
                            l_coeffs_flags |= DRS_COEF_K;
                        else if( dap_strcmp(l_coeffs_strs[i], "B") == 0 )
                            l_coeffs_flags |= DRS_COEF_B;
                        else if( dap_strcmp(l_coeffs_strs[i], "K9") == 0 )
                            l_coeffs_flags |= DRS_COEF_K9;
                        else if( dap_strcmp(l_coeffs_strs[i], "B9") == 0 )
                            l_coeffs_flags |= DRS_COEF_B9;
                        else {
                            log_it(L_ERROR, "Unknown flag %s", l_coeffs_strs[i]);
                        }
                        DAP_DELETE(l_coeffs_strs[i]);
                    }
                    DAP_DELETE(l_coeffs_strs);
                }
            }
            if (l_drs_num == -1){ // ???? ?? ?????? DRS ?????, ?? ??????? ???
                for (int i = 0; i < DRS_COUNT; i++){
                    drs_calibrate_t *l_cal = drs_calibrate_get_state(i);
                    dap_string_append_printf(l_reply, "--== DRS %d ==--\n", i);
                    s_calibrate_state_print(l_reply, l_cal, l_limits, l_coeffs_flags);
                }
            }else{ // ???? ??????, ?? ?????? ??????????
                drs_calibrate_t * l_cal = drs_calibrate_get_state(l_drs_num);
                dap_string_append_printf(l_reply, "--== DRS %d ==--\n", l_drs_num);
                s_calibrate_state_print(l_reply, l_cal, l_limits, l_coeffs_flags);
            }
            *a_str_reply = dap_string_free(l_reply, false);
        } else if ( dap_strcmp( a_argv[1], "abort") == 0){
            dap_string_t * l_reply = dap_string_new("Abort drs calibration:\n");
            if (l_drs_num == -1){ // ???? ?? ?????? DRS ?????, ?? ??????? ???
                for (int i = 0; i < DRS_COUNT; i++){
                    int l_ret = drs_calibrate_abort(i);
                    if (l_ret == 0)
                        dap_string_append_printf(l_reply, "DRS #%d calibration aborted\n", i);
                    else
                        dap_string_append_printf(l_reply, "DRS #%d calibration abort error, code %d\n", i, l_ret);
                }
            }else{ // ???? ??????, ?? ?????? ??????????
                int l_ret = drs_calibrate_abort(l_drs_num);
                if (l_ret == 0)
                    dap_string_append_printf(l_reply, "DRS #%d calibration aborted\n", l_drs_num);
                else
                    dap_string_append_printf(l_reply, "DRS #%d calibration abort error, code %d\n", l_drs_num, l_ret);
            }
            *a_str_reply = dap_string_free(l_reply, false);
        } else {
            dap_cli_server_cmd_set_reply_text(a_str_reply, "Unknown subcommand \"%s\"\n", a_argv[1]);
        }
        return 0;
    } else{
        dap_cli_server_cmd_set_reply_text(a_str_reply, "No subcommand. Availble subcommands: run, check. Do \"help calibrate\" for more details\n");
        return -1;
    }
}

#define dap_string_append_array(a_reply, a_name, a_fmt, a_array, a_limits )\
    {\
        size_t l_array_count = (sizeof(a_array))/sizeof(a_array[0]); \
        size_t l_limit = a_limits == 0 || a_limits > l_array_count ? l_array_count  : a_limits; \
        dap_string_append_printf(a_reply,"%s:{",a_name);\
        for (size_t i = 0; i < l_limit ; i++){\
            dap_string_append_printf(a_reply, a_fmt, a_array[i]); \
            if (i != l_limit-1 ) \
                dap_string_append_printf(a_reply, ", "); \
            if ( i && i % 16 == 0 ) \
                dap_string_append_printf(a_reply, "\n"); \
        } \
        dap_string_append_printf(a_reply,"}\n");\
    }


/**
 * @brief s_calibrate_state_print
 * @param a_reply
 * @param a_cal
 * @param a_limits
 * @param a_flags
 */
static void s_calibrate_state_print(dap_string_t * a_reply, drs_calibrate_t *a_cal, unsigned a_limits, int a_flags)
{
    pthread_rwlock_rdlock(&a_cal->rwlock);
    dap_string_append_printf( a_reply, "Running:     %s\n", a_cal->is_running? "yes" : "no" );
    dap_string_append_printf( a_reply, "Progress:    %d%%\n", a_cal->progress );
    if (a_cal->ts_end){
        coefficients_t * l_params = &a_cal->drs->coeffs;
        if ( a_flags & DRS_COEF_SPLASH)
            dap_string_append_array(a_reply, "splash", "0x%08X", l_params->splash, a_limits);


        if ( a_flags & DRS_COEF_DELTA_TIME )
            dap_string_append_array(a_reply, "deltaTimeRef", "%f", l_params->deltaTimeRef, a_limits);

        if ( a_flags & DRS_COEF_CHAN_K )
            dap_string_append_array(a_reply, "chanK", "%f", l_params->chanK, a_limits);

        if ( a_flags & DRS_COEF_CHAN_B )
            dap_string_append_array(a_reply, "chanB", "%f", l_params->chanB, a_limits);

        if ( a_flags & DRS_COEF_K_TIME )
            dap_string_append_array(a_reply, "kTime", "%f", l_params->kTime, a_limits);

        if ( a_flags & DRS_COEF_K ){
            char l_str[128];
            for(unsigned c = 0; c< DRS_CHANNELS_COUNT; c++){
                snprintf(l_str, sizeof(l_str),"k[%u]", c);
                dap_string_append_array(a_reply, l_str, "%f", l_params->k[c], a_limits);
            }
        }

        if ( a_flags & DRS_COEF_B ){
            char l_str[128];
            for(unsigned c = 0; c< DRS_CHANNELS_COUNT; c++){
                snprintf(l_str, sizeof(l_str),"b[%u]", c);
                dap_string_append_array(a_reply, l_str, "%f", l_params->b[c], a_limits);
            }
        }

        if ( a_flags & DRS_COEF_K9 )
            dap_string_append_array(a_reply, "k9", "%f", l_params->k9, a_limits);

        if ( a_flags & DRS_COEF_B9 )
            dap_string_append_array(a_reply, "b9", "%f", l_params->b9, a_limits);

        dap_string_append_printf( a_reply, "indicator=%d \n", l_params->indicator);
        dap_string_append_printf( a_reply, "Got time:    %.3f seconds \n",
                                   ((double) (a_cal->ts_end - a_cal->ts_start)) / 1000000000.0 );


    }
    pthread_rwlock_unlock(&a_cal->rwlock);
}

/**
 * @brief s_callback_help
 * @param argc
 * @param argv
 * @param str_reply
 * @return
 */
static int s_callback_help(int argc, char ** argv, char **str_reply)
{
    if(argc > 1) {
        log_it(L_DEBUG, "Help for command %s", argv[1]);
        dap_cli_cmd_t *l_cmd = dap_cli_server_cmd_find(argv[1]);
        if(l_cmd) {
            dap_cli_server_cmd_set_reply_text(str_reply, "%s:\n%s", l_cmd->doc, l_cmd->doc_ex);
            return 0;
        } else {
            dap_cli_server_cmd_set_reply_text(str_reply, "command \"%s\" not recognized", argv[1]);
        }
        return -1;
    } else {
        // TODO Read list of commands & return it
        log_it(L_DEBUG, "General help requested");
        dap_string_t * l_help_list_str = dap_string_new(NULL);
        dap_cli_cmd_t *l_cmd = dap_cli_server_cmd_get_first();
        while(l_cmd) {
            dap_string_append_printf(l_help_list_str, "%s:\t\t\t%s\n",
                    l_cmd->name, l_cmd->doc ? l_cmd->doc : "(undocumented command)");
            l_cmd = (dap_cli_cmd_t*) l_cmd->hh.next;
        }
        dap_cli_server_cmd_set_reply_text(str_reply,
                "Available commands:\n\n%s\n",
                l_help_list_str->len ? l_help_list_str->str : "NO ANY COMMAND WERE DEFINED");
        return 0;
    }
}

/**
 * @brief s_callback_exit
 * @param argc
 * @param argv
 * @param str_reply
 * @return
 */
static int s_callback_exit(int argc, char ** argv, char **str_reply)
{
    UNUSED(argc);
    UNUSED(argv);
    UNUSED(str_reply);
    exit(0);
}
