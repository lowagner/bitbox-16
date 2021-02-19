#include "common.h"

void set_game_message_timeout(const char *msg, int timeout)
{
    strcpy((char *)game_message, msg);
    game_message_timeout = timeout;
}
