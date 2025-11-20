#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "trie.h"
#include "cbf.h"
#include "utils.h"

#define DNS_PORT 53300
#define BUFFER_SIZE 65536

static int create_listener(void) {
    int sockfd;
    struct sockaddr_in server_addr;

    // 1. Create UDP Socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Configure Server Address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DNS_PORT);

    // 3. Bind Socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed (Run with sudo?)");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void parse_file(trieNode* tree, CBF* cbf) {
    // parse from hosts.txt
    char data[256];
    FILE *ptr = fopen("hosts.txt", "r");
    while (fgets(data, sizeof(data), ptr)) {
        char first[128], second[128];
        if (sscanf(data, "%127s %127s", first, second) == 2) {
            uint32_t ip;
            inet_pton(AF_INET, second, &ip);
            trie_insert(tree, first, ip);
            cbf_insert(cbf, first);
            printf("first = %s, second = %s\n", first, second);
        } else {
            fprintf(stderr, "failed to parse line: %s", data);
        }
    }
    fclose(ptr);
}

void send_nxdomain_response(
    int sockfd,
    const struct sockaddr *client_addr,
    socklen_t client_len,
    const uint8_t *query,
    size_t query_len
) {
    uint8_t response[512];
    memset(response, 0, sizeof(response));

    if (query_len < sizeof(struct DNS_HEADER)) {
        // Malformed query
        return;
    }

    struct DNS_HEADER *qhdr = (struct DNS_HEADER *)query;
    struct DNS_HEADER *rhdr = (struct DNS_HEADER *)response;

    // Copy ID from query
    rhdr->id = qhdr->id;

    // Parse RD from original query flags (in network byte order)
    uint16_t qflags = ntohs(qhdr->flags);
    int rd = (qflags & 0x0100) ? 1 : 0;  // RD is bit 8

    // Build response flags:
    // QR = 1 (response), OPCODE = 0, AA = 0, TC = 0, RD = from query
    // RA = 0, Z = 0, RCODE = 3 (NXDOMAIN)
    uint16_t rflags = 0;
    rflags |= 0x8000;        // QR = 1 (response)
    if (rd) {
        rflags |= 0x0100;    // RD (copy from query)
    }
    rflags |= 0x0003;        // RCODE = 3 (NXDOMAIN)

    rhdr->flags     = htons(rflags);
    rhdr->q_count   = htons(1);  // one question
    rhdr->ans_count = htons(0);  // no answers
    rhdr->auth_count= htons(0);  // no authority RRs
    rhdr->add_count = htons(0);  // no additional RRs

    // Question section starts immediately after the 12-byte header
    size_t qname_offset = sizeof(struct DNS_HEADER);
    if (query_len < qname_offset + 1) {
        // not enough data
        return;
    }

    size_t offset = qname_offset;

    // Walk QNAME (series of [len][label bytes] ... [0])
    while (offset < query_len && query[offset] != 0) {
        uint8_t label_len = query[offset];

        // basic sanity check
        if (label_len == 0 || offset + 1 + label_len >= query_len) {
            return;
        }

        offset += 1 + label_len;
    }

    // Need: 0 byte + QTYPE(2) + QCLASS(2)
    if (offset + 1 + 4 > query_len) {
        return;
    }

    offset++; // skip the 0 terminator

    // Total question length = QNAME + 0 + QTYPE + QCLASS
    size_t question_len = offset + 4 - qname_offset;

    if (sizeof(response) < sizeof(struct DNS_HEADER) + question_len) {
        return;
    }

    // Copy question from query to response right after header
    memcpy(response + sizeof(struct DNS_HEADER),
           query + qname_offset,
           question_len);

    size_t resp_len = sizeof(struct DNS_HEADER) + question_len;

    ssize_t sent = sendto(sockfd,
                          response,
                          resp_len,
                          0,
                          client_addr,
                          client_len);
    if (sent < 0) {
        perror("sendto NXDOMAIN");
    }
}

int main(void) {
    trieNode* tree = trie_create_node();
    CBF* cbf = cbf_create(100, .01);

    parse_file(tree, cbf);

    int sockfd;
    unsigned char buffer[BUFFER_SIZE];

    sockfd = create_listener();
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    printf("DNS Server listening on port %d...\n", DNS_PORT);

    while (1) {
        // 4. Receive DNS Query
        int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&client_addr, &addr_len);
        if (n < 0) continue;

        struct DNS_HEADER *dns = (struct DNS_HEADER *)buffer;

        // Point to the Query Name part (immediately after header)
        unsigned char *qname = (unsigned char *)(buffer + sizeof(struct DNS_HEADER));
        
        char domain[256];
        qname_to_string(qname, domain, sizeof(domain));
        printf("%s\n", domain);

        if (cbf_possibly_exists(cbf, domain) == 0) {
            send_nxdomain_response(
                sockfd,
                (struct sockaddr *)&client_addr, 
                addr_len,
                buffer, 
                (size_t)n
            );
            continue;
        }

        uint32_t ip;
        if (trie_lookup(tree, domain, &ip) == 1) {
            char buf[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &ip, buf, sizeof buf)) {
                printf("%s -> %s\n", domain, buf);
            }
        } else {
            send_nxdomain_response(
                sockfd,
                (struct sockaddr *)&client_addr, 
                addr_len,
                buffer, 
                (size_t)n
            );
            continue;
        }
        
        // Move pointer past the qname (variable length) to get to QTYPE
        // DNS names are length-prefixed labels ending with 0
        unsigned char *reader = qname;
        while (*reader != 0) {
            reader++;
        }
        reader++; // Step over the final 0

        struct R_DATA *qinfo = (struct R_DATA *)reader;
        
        // 5. Filter: Only Handle 'A' Records (Type 1)
        // ntohs converts Network Byte Order (Big Endian) to Host Byte Order
        unsigned short qtype = ntohs(*(unsigned short*)reader);
        reader += 2; // Jump over QTYPE
        unsigned short qclass = ntohs(*(unsigned short*)reader); // QCLASS
        
        if (qtype == 1) {
            printf("Received A Record Query. Responding...\n");

            // 6. Construct Response Packet
            // We reuse the 'buffer' because the Question section must be echoed back
            
            // -- Modify Header --
            uint16_t qflags = ntohs(dns->flags);
            int rd = (qflags & 0x0100) ? 1 : 0;

            uint16_t rflags = 0;
            rflags |= 0x8000;
            if (rd) {
                rflags |= 0x0100;
            }
            dns->flags = htons(rflags); // Standard Response, No Error
            dns->q_count = htons(1); // one question
            dns->ans_count = htons(1);  // We are providing 1 answer
            dns->auth_count = htons(0); // zero authority
            dns->add_count = htons(0); // zero additional
            // q_count remains 1, others remain 0

            // -- Construct Answer Section --
            // Pointer is currently at the end of the Question Section.
            // We append the answer there.
            unsigned char *response_ptr = reader + 2; // Skip over QCLASS bytes from read above

            // NAME: Pointers in DNS allow compression. 
            // 0xC00C points to offset 12 (Start of the packet body / Question Name)
            unsigned short name_ptr = htons(0xC00C); 
            memcpy(response_ptr, &name_ptr, 2);
            response_ptr += 2;

            // TYPE: A Record (1)
            unsigned short type = htons(1);
            memcpy(response_ptr, &type, 2);
            response_ptr += 2;

            // CLASS: IN (1)
            unsigned short _class = htons(1);
            memcpy(response_ptr, &_class, 2);
            response_ptr += 2;

            // TTL: Time to Live (e.g., 300 seconds)
            unsigned int ttl = htonl(300);
            memcpy(response_ptr, &ttl, 4);
            response_ptr += 4;

            // RDLENGTH: Length of Data (IPv4 = 4 bytes)
            unsigned short data_len = htons(4);
            memcpy(response_ptr, &data_len, 2);
            response_ptr += 2;

            // RDATA: The IP Address
            struct in_addr ip_addr;

            memcpy(response_ptr, &ip, 4);
            response_ptr += 4;

            // 7. Send Response
            int packet_len = response_ptr - buffer;
            sendto(sockfd, (const char *)buffer, packet_len, 0,
                   (const struct sockaddr *)&client_addr, addr_len);
        } else {
            printf("Ignored non-A record query (Type: %d)\n", qtype);
        }
    }
    cbf_free(cbf);
    trie_free(tree);
    return 0;
}
