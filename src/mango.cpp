#include "mango.hpp"

#include <cstring>
#include <iostream>

#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

namespace mango
{

void send_cmd(struct mangoapp_ctrl_msgid1_v1 *msg)
{
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    int key = ftok("/usr/bin/mangoapp", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    msgsnd(msgid, &msg, sizeof(struct mangoapp_ctrl_msgid1_v1), IPC_NOWAIT);
#endif
}

void stop_logging()
{
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    struct mangoapp_ctrl_msgid1_v1 msg = {
        .hdr.msg_type = 2,
        .hdr.ctrl_msg_type = 1,
        .hdr.version = 1,

        .no_display = 0,         // ignore/no-change
        .log_session = 2,        // stop session
        .log_session_name = {0}, // name is unused for this
        .reload_config = 0,      // don't reload config
    };
    send_cmd(&msg);
#endif
}

void start_logging(const char *name)
{
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    struct mangoapp_ctrl_msgid1_v1 msg = {
        .hdr.msg_type = 2,
        .hdr.ctrl_msg_type = 1,
        .hdr.version = 1,

        .no_display = 0,         // ignore/no-change
        .log_session = 1,        // start session
        .log_session_name = {0}, // initialize to zero
        .reload_config = 0,      // don't reload config
    };
    strncpy(msg.log_session_name, name, sizeof(msg.log_session_name));
    send_cmd(&msg);
#endif
}

}; // namespace mango
