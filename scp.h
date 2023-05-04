#ifndef SCP_H
#define SCP_H

#include <String.h>
#include <Arduino.h>
#include "userdefine.h"

#define CMD_TBL_SIZE 3
#define MAX_MESSAGE_LENGTH 22
#define VALID_RES_BUF_SIZE 8
#define LOG_ENTRY_SIZE 32

struct scp_cmd_tbl_s
{
    unsigned char command_code;
    callback_fn getter_fn;
    callback_fn setter_fn;
};

char *buffer_read(char *str_buffer);
int check_action_command(char *from_data_received);
// int exec_command(char *from_data_filtered, int from_data_action_command);

#endif