#include <stddef.h>

// DNS Header Structure (12 bytes)
// We use __attribute__((packed)) to prevent compiler padding
struct DNS_HEADER {
    unsigned short id;          // Identification number
    unsigned short flags;       // DNS Flags
    unsigned short q_count;     // Number of Questions
    unsigned short ans_count;   // Number of Answer RRs
    unsigned short auth_count;  // Number of Authority RRs
    unsigned short add_count;   // Number of Additional RRs
} __attribute__((packed));

// Resource Record structure (Answer section format)
struct R_DATA {
    unsigned short type;
    unsigned short _class;
    unsigned int   ttl;
    unsigned short data_len;
} __attribute__((packed));

static inline char* qname_to_string(const unsigned char* qname, char *out, size_t out_size) {
    int pos = 0;
    const unsigned char *p = qname;

    while (*p != 0) {
        unsigned int len = *p++;

        if (len == 0 || pos + (int)len + 1 >= (int)out_size) {
            break;
        }

        for (unsigned int i = 0; i < len; i++) {
            out[pos++] = (char)*p++;
        }

        out[pos++] = '.';
    }

    if (pos == 0) {
        // empty name?
        out[pos] = '\0';
    } else {
        // overwrite the last dot with string terminator
        out[pos - 1] = '\0';
    }

    return out;
}