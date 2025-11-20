#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DNS_PORT 53
#define BUFFER_SIZE 65536
#define RESPONSE_IP "192.168.1.100" 

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

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    unsigned char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(client_addr);

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

    printf("DNS Server listening on port %d...\n", DNS_PORT);

    while (1) {
        // 4. Receive DNS Query
        int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0,
                         (struct sockaddr *)&client_addr, &addr_len);
        if (n < 0) continue;

        struct DNS_HEADER *dns = (struct DNS_HEADER *)buffer;

        // Point to the Query Name part (immediately after header)
        unsigned char *qname = (unsigned char *)(buffer + sizeof(struct DNS_HEADER));
        
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
            dns->flags = htons(0x8180); // Standard Response, No Error
            dns->ans_count = htons(1);  // We are providing 1 answer
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
            inet_pton(AF_INET, RESPONSE_IP, &ip_addr);
            memcpy(response_ptr, &ip_addr, 4);
            response_ptr += 4;

            // 7. Send Response
            int packet_len = response_ptr - buffer;
            sendto(sockfd, (const char *)buffer, packet_len, 0,
                   (const struct sockaddr *)&client_addr, addr_len);
        } else {
            printf("Ignored non-A record query (Type: %d)\n", qtype);
        }
    }
    return 0;
}
