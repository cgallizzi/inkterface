#ifndef MANGO_HPP
#define MANGO_HPP

#include <stdint.h>

struct mangoapp_msg_header {
    long msg_type;    // Message queue ID, never change
    uint32_t version; /* for major changes in the way things work */
} __attribute__((packed));

struct mangoapp_msg_v1 {
    struct mangoapp_msg_header hdr;

    uint32_t pid;
    uint64_t visible_frametime_ns;
    uint8_t fsrUpscale;
    uint8_t fsrSharpness;
    // For debugging
    uint64_t app_frametime_ns;
    uint64_t latency_ns;
    uint32_t outputWidth;
    uint32_t outputHeight;
    // WARNING: Always ADD fields, never remove or repurpose fields
} __attribute__((packed));

void msg_read_thread();

#endif /* MANGO_HPP */
