#include "mango.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>

#if defined(MANGO_PROTO) && !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

namespace mango
{

void send_cmd([[maybe_unused]] struct mangoapp_ctrl_msgid1_v1 *msg)
{
#if defined(MANGO_PROTO) && !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    int key = ftok("mangoapp", 65);
    int msgid = msgget(key, 0666 | IPC_CREAT);
    std::cout << "send_cmd() key: " << key << ", msgid: " << msgid << std::endl;
    msgsnd(msgid, &msg, sizeof(struct mangoapp_ctrl_msgid1_v1), IPC_NOWAIT);
#endif
}

void stop_logging()
{
#if defined(MANGO_PROTO) && !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    struct mangoapp_ctrl_msgid1_v1 msg;
        msg.hdr.msg_type = 2;
        msg.hdr.ctrl_msg_type = 1;
        msg.hdr.version = 1;

        msg.no_display = 0;         // ignore/no-change
        msg.log_session = 2;        // stop session
        msg.log_session_name[0] = 0; // name is unused for this
        msg.reload_config = 0;      // don't reload config
    send_cmd(&msg);
#else
    system("mangohudctl set log_session false");
#endif
}

void start_logging([[maybe_unused]] const char *name)
{
#if defined(MANGO_PROTO) && !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
    struct mangoapp_ctrl_msgid1_v1 msg;
        msg.hdr.msg_type = 2;
        msg.hdr.ctrl_msg_type = 1;
        msg.hdr.version = 1;

        msg.no_display = 0;         // ignore/no-change
        msg.log_session = 1;        // start session
        msg.log_session_name[0] = 0; // initialize to zero 
        msg.reload_config = 0;      // don't reload config
    strncpy(msg.log_session_name, name, sizeof(msg.log_session_name));
    send_cmd(&msg);
#else
    system("mangohudctl set log_session true");
#endif
}

}; // namespace mango
