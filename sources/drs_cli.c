/*
 * drs_proto_cmd.h
 *
 *  Created on: 1 November 2022
 *      Author: Dmitriy Gerasimov <dmitry.gerasimov@demlabs.net>
 */

#include <dap_common.h>
#include <dap_string.h>
#include <dap_cli_server.h>

#include "drs_cli.h"

#include "calibrate.h"

#define LOG_TAG "drs_cli"

static int s_callback_calibrate(int argc, char ** argv, char **str_reply);
static int s_callback_help(int argc, char ** argv, char **str_reply);
static int s_callback_exit(int argc, char ** argv, char **str_reply);

static void s_calibrate_state_print(calibrate_state_t *a_state, dap_string_t * a_reply);

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

    return 0;
}

/**
 * @brief drs_cli_deinit
 */
void drs_cli_deinit()
{

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

        // „итаем доп аргумент (если есть)
        int l_drs_num = -1; // -1 значит дл€ всех
        if(argc > 2) {
            l_drs_num = atoi(argv[2]);
        }

        if (strcmp(argv[1], "run") == 0 ){ // Subcommand "run"
            if (l_drs_num == -1) // ≈сли не указан DRS канал, то фигачим все
                for (int i = 0; i < DRS_COUNT; i++)
                    calibrate_run(i);
            else // ≈сли указан, то только конкретный
                calibrate_run(l_drs_num);
        }else if  (strcmp(argv[1], "check") == 0 ){ // Subcommand "check"
            dap_string_t * l_reply = dap_string_new("Check drs state:\n");
            if (l_drs_num == -1){ // ≈сли не указан DRS канал, то фигачим все
                for (int i = 0; i < DRS_COUNT; i++){
                    calibrate_state_t *l_state = calibrate_get_state(i);
                    dap_string_append_printf(l_reply, "--== DRS %d ==--\n", i);
                    s_calibrate_state_print(l_state, l_reply);
                }
            }else{ // ≈сли указан, то только конкретный
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
