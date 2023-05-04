#ifndef USERDEFINE_H
#define USERDEFINE_H

typedef int (*callback_fn) (uint8_t *);
// ptr ==> Starting address of memory to be filled
// x   ==> Value to be filled
// n   ==> Number of bytes to be filled starting
//         from ptr to be filled
typedef struct {
	uint8_t rcv_done;
	char *start_address;
	char *stop_address;
	char buffer_ptr[255];
} uart_handler_t;

typedef enum {
	ACTION_COMMAND_GET,
	ACTION_COMMAND_SET,
	ACTION_COMMAND_INVALID,
} gui_action_command_e;

typedef enum {
  INVALID_CMD,
  VALID_CMD,
} cmd_validator_ret_e;

#endif