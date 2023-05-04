#include "scp.h"

uart_handler_t uart_gui_hndl;
extern const struct scp_cmd_tbl_s logger_cmd_tbl_s[CMD_TBL_SIZE];

char *buffer_read(char *str_buffer)
{
    char *data_rcvd;
    /* declare start and end pointer that have same address as s pointer */
    uart_gui_hndl.start_address = str_buffer;
    uart_gui_hndl.stop_address = str_buffer;
    /* check if *s is true */
    while(*str_buffer)
    {
    	if(*str_buffer != '{' && *str_buffer != '}') str_buffer++;
    	/* check if there is bracket { inside of buffer pointer */
        if(*str_buffer == '{') uart_gui_hndl.start_address = str_buffer;
        /* check if there is bracket } inside of buffer pointer */
        if(*str_buffer == '}') uart_gui_hndl.stop_address = str_buffer;
        /* make sure that start address is before end address */
        if(uart_gui_hndl.start_address < uart_gui_hndl.stop_address && (*(uart_gui_hndl.start_address)))
        {
            (*(uart_gui_hndl.stop_address)) = 0x0;
            /* address of pointer data_rcv is address of pointer start, +1 */
            data_rcvd = uart_gui_hndl.start_address+1;
            uart_gui_hndl.start_address = str_buffer = uart_gui_hndl.stop_address;
        };
        str_buffer++;
    }
    return data_rcvd;
}

int check_action_command(char *from_data_received)
{
  if ( (*(from_data_received+1)) != '?' && (*(from_data_received+1)) != ':' ) return ACTION_COMMAND_INVALID;

	if ( (*(from_data_received+1)) == '?' && (*(from_data_received+2)) ) return ACTION_COMMAND_INVALID;

	if ( (*(from_data_received+1)) == '?' )
    {
        return ACTION_COMMAND_GET;
    }

	else if ( (*(from_data_received+1)) == ':' )
    {
        return ACTION_COMMAND_SET;
    }

	return ACTION_COMMAND_INVALID;
}
