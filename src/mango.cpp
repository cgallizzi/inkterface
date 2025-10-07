#include "mango.hpp"

#include <iostream>

#if defined(Q_OS_LINUX)
#include <sys/ipc.h>
#include <sys/msg.h>
#endif

void msg_read_thread()
{
#if defined(Q_OS_LINUX)
    int key = ftok("mangoapp", 65);
    msgid = msgget(key, 0666 | IPC_CREAT);
    const struct mangoapp_msg_header *hdr = (const struct mangoapp_msg_header *)raw_msg;
    const struct mangoapp_msg_v1 *mangoapp_v1 = (const struct mangoapp_msg_v1 *)raw_msg;

    while (1) {
        size_t msg_size = msgrcv(msgid, (void *)raw_msg, sizeof(raw_msg), 1, 0);
        if (msg_size != size_t(-1)) {
            if (hdr->version == 1) {
                if (msg_size > offsetof(struct mangoapp_msg_v1, visible_frametime_ns)) {
                    std::cout << "got frametime: " << mangoapp_v1->visible_frametime_ns
                              << std::endl;
                }
            }
        }
    }
#endif
}
