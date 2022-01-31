#ifndef NETWORK_H
#define NETWORK_H

/*
 * A system to handle network connections and trigger events whenever data can
 * be read/written from/to a socket, both TCP or UDP supported.
 */

#include <thirty/dsutils.h>
#include <pthread.h>
#include <poll.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * Data structure defining a networking error. Use network_retrieveError to get
 * a nice error message.
 */
struct network_error {
        enum {
                NETWORK_ERROR_SYSCALL,
                NETWORK_ERROR_GETADDRINFO,
                NETWORK_ERROR_LOCAL,
        } type;
        const char *str;
        int code;
} errors;

/*
 * Data structure defining networking and keeping track of the current status.
 */
struct network {
        bool use_tcp_socket;
        bool use_udp_socket;
        int tcp_socket;
        int udp_socket;
        bool tcpTriggerWrite;
        bool udpTriggerWrite;
        struct pollfd fds[2];
        struct growingArray errors;
};

/*
 * Initialize networking data structure. Set up whether it will use a TCP
 * and/or a UDP socket. If both tcp and udp are false, behavior when
 * network_run is called is undefined. If [tcp|udp]Triggeronwriteable is true,
 * by default network will trigger an event when the socket can be written
 * to. If false, this behavior won't be set by default. The behavior can be
 * changed while network_run is running from a callback by calling
 * network_setTriggerOnWriteable[TCP|UDP]. Note that the behavior only changes
 * after poll has finished its current call.
 */
void network_init(struct network *network,
                  bool tcpTriggerOnWriteable, bool udpTriggerOnWriteable,
                  bool use_tcp, bool use_udp);

/*
 * Connect to a server. Return true if successful. If false is returned, use
 * network_retrieveError to retrieve error information. This function should be
 * called only once.
 */
bool network_connect(struct network *network, const char *host, unsigned port);

/*
 * Start the networking polling loop. This function only returns either in case
 * of error (false) or if a signal is received (true). If it returns due to an
 * error, use network_retrieveError to retrieve error information. While
 * running, this function may trigger events (see
 * eventBroker). network_setTriggeronwriteable can be called from a callback
 * while this is running to change whether to react to writteable sockets.
 */
bool network_run(struct network *network);

/*
 * Sets whether an event will be triggered when the TCP or UDP socket becomes
 * writeable. If network is not using this type of socket this function does
 * nothing. This changes the behavior when network_run is first called as well
 * as while it is running and this is called from a callback. However, it
 * should be noted that the change will only take place after the current call
 * to poll() has finished. Therefore, it might be convinient to interrupt it by
 * sending an inocuous signal if the callback is deferred and not called
 * synchronously (see eventBroker).
 */
void network_setTriggerOnWriteableTCP(struct network *network, bool value);
void network_setTriggerOnWriteableUDP(struct network *network, bool value);

/*
 * Write the latest error to occurr onto a buffer pointed to by msg with length
 * len. If no error occurred, write "no error".
 */
void network_retrieveError(struct network *network, char *msg, size_t len);

/*
 * Free all resources used by network data structure.
 */
void network_destroy(struct network *network);

#endif
