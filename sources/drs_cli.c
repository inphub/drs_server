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
#include "calibrate.h"

#define LOG_TAG "drs_cli"

static int s_callback_init(int a_argc, char ** a_argv, char **a_str_reply);
static int s_callback_start(int a_argc, char ** a_argv, char **a_str_reply);
static int s_callback_read(int a_argc, char ** a_argv, char **a_str_reply);
static int s_callback_calibrate(int argc, char ** argv, char **str_reply);
static int s_callback_help(int argc, char ** argv, char **str_reply);
static int s_callback_exit(int argc, char ** argv, char **str_reply);

static void s_calibrate_state_print(calibrate_state_t *a_state, dap_string_t * a_reply);
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
                "calibrate run [<DRS number>]\n"
                "\tRun calibration process for specified DRS number or for all channels if number is not specified\n"
                "\n"
                "calibrate check [<DRS number>]\n"
                "\tCheck calibration status for specified DRS number or for all channels if number is not specified\n"
                "\n"
                           );

    // Get raw data
    dap_cli_server_cmd_add ("read", s_callback_read, "Read raw data ",
                            "\n"
                            "read write_ready -drs <DRS num>"
                            "\n"
                            ""
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
        // Проверяем корректность ввода
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
    // Инициализация DRS
    if(drs_init(NULL) != 0){
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

    // Получаем номер DRS
    int l_drs_num = s_parse_drs_and_check(l_arg_index, a_argc, a_argv, a_str_reply);
    if (l_drs_num < -1 ) // Wrong DRS num
        return -1;

    switch (l_cmd_num){
        case CMD_WRITE_READY:{
                bool l_flag_ready=drs_get_flag_write_ready(l_drs_num);
                dap_cli_server_cmd_set_reply_text( a_str_reply, "DRS #%d is %s", l_flag_ready? "ready": "not ready");
        }break;
        case CMD_PAGE:{
            if(l_drs_num!=-1){
                memset(tmasFast, 0, sizeof(tmasFast));
                dap_string_t * l_reply = dap_string_new("");
                dap_string_append_printf(l_reply,"Page read for DRS %d\n", l_drs_num);
                drs_t * l_drs = &g_drs[l_drs_num];
                drs_data_get(l_drs,tmasFast,0);
                for (size_t t = 0; t < 1000; t++){
                    dap_string_append_printf(l_reply, "0x%02X ", tmasFast[t]);
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
                    drs_data_get(l_drs,tmasFast,0);
                    for (size_t t = 0; t < 1000; t++){
                        dap_string_append_printf(l_reply, "%02X ", tmasFast[t]);
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
static int s_callback_calibrate(int argc, char ** argv, char **str_reply)
{
    if(argc > 1) {

        // Читаем доп аргумент (если есть)
        int l_drs_num = -1; // -1 значит для всех
        if(argc > 2) {
            l_drs_num = atoi(argv[2]);
        }

        if (strcmp(argv[1], "run") == 0 ){ // Subcommand "run"
            if (l_drs_num == -1) // Если не указан DRS канал, то фигачим все
                for (int i = 0; i < DRS_COUNT; i++)
                    calibrate_run(i);
            else // Если указан, то только конкретный
                calibrate_run(l_drs_num);
        }else if  (strcmp(argv[1], "check") == 0 ){ // Subcommand "check"
            dap_string_t * l_reply = dap_string_new("Check drs state:\n");
            if (l_drs_num == -1){ // Если не указан DRS канал, то фигачим все
                for (int i = 0; i < DRS_COUNT; i++){
                    calibrate_state_t *l_state = calibrate_get_state(i);
                    dap_string_append_printf(l_reply, "--== DRS %d ==--\n", i);
                    s_calibrate_state_print(l_state, l_reply);
                }
            }else{ // Если указан, то только конкретный
                calibrate_state_t * l_state = calibrate_get_state(l_drs_num);
                dap_string_append_printf(l_reply, "--== DRS %d ==--\n", l_drs_num);
                s_calibrate_state_print(l_state, l_reply);
            }
            *str_reply = dap_string_free(l_reply, false);
        }
        return 0;
    } else{
        dap_cli_server_cmd_set_reply_text(str_reply, "No subcommand. Availble subcommands: run, check. Do \"help calibrate\" for more details");
        return -1;
    }
}

/**
 * @brief s_calibrate_state_print
 * @param a_state
 * @param a_reply
 */
static void s_calibrate_state_print(calibrate_state_t *a_state, dap_string_t * a_reply)
{
    dap_string_append_printf( a_reply, "Running:     %s\n", a_state? "yes" : "no" );
    if (a_state){
        dap_string_append_printf( a_reply, "Progress:    %d%%\n", a_state->progress );
    }
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
