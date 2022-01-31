#define _GNU_SOURCE

#include <thirty/network.h>
#include <thirty/eventBroker.h>
#include <thirty/util.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <poll.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

static inline bool disable_all_signals(sigset_t *prev) {
        sigset_t all_signals;
        sigfillset(&all_signals);
        int err = pthread_sigmask(SIG_SETMASK, &all_signals, prev);
        if (err != 0) {
                errno = err;
                return false;
        }
        return true;
}

static inline void add_syscall_error(struct network *network, const char *name) {
        struct network_error *err = growingArray_append(&network->errors);
        err->type = NETWORK_ERROR_SYSCALL;
        err->str = name;
        err->code = errno;
}

static inline void add_getaddr_error(struct network *network, int code) {
        struct network_error *err = growingArray_append(&network->errors);
        err->type = NETWORK_ERROR_GETADDRINFO;
        err->str = NULL;
        err->code = code;
}

static inline void add_local_error(struct network *network, const char *error) {
        struct network_error *err = growingArray_append(&network->errors);
        err->type = NETWORK_ERROR_LOCAL;
        err->str = error;
}

static int get_connected_socket(struct network *network, const char *host, const char *service, struct addrinfo *hints) {
        struct addrinfo *res;
        int err = getaddrinfo(host, service, hints, &res);

        if (err != 0) {
                if (err == EAI_SYSTEM) {
                        add_syscall_error(network, "getaddrinfo");
                } else {
                        add_getaddr_error(network, err);
                }
                return -1;
        }

        int fd = -1;
        for (struct addrinfo *node = res; node != NULL; node = node->ai_next) {
                fd = socket(node->ai_family, node->ai_socktype | SOCK_NONBLOCK, node->ai_protocol);
                if (fd == -1) {
                        add_syscall_error(network, "socket");
                        continue;
                }
                if (connect(fd, node->ai_addr, node->ai_addrlen) == -1) {
                        add_syscall_error(network, "connect");
                        if (close(fd) == -1) {
                                add_syscall_error(network, "close");
                        }
                        fd = -1;
                        continue;
                }
                break;
        }

        if (fd != -1) {
                growingArray_clear(&network->errors);
        }

        freeaddrinfo(res);
        return fd;
}

void network_init(struct network *network, bool tcpTriggerOnWriteable, bool udpTriggerOnWriteable, bool use_tcp, bool use_udp) {
        growingArray_init(&network->errors, sizeof(struct network_error), 4);
        network->use_tcp_socket = use_tcp;
        network->use_udp_socket = use_udp;
        network->tcpTriggerWrite = tcpTriggerOnWriteable;
        network->udpTriggerWrite = udpTriggerOnWriteable;
}

bool network_connect(struct network *network, const char *host, unsigned port) {
        struct addrinfo hints = {
                .ai_flags = AI_V4MAPPED | AI_ADDRCONFIG | AI_NUMERICSERV,
                .ai_family = AF_UNSPEC,
                .ai_protocol = 0,
                .ai_addrlen = 0,
                .ai_addr = NULL,
                .ai_canonname = NULL,
                .ai_next = NULL,
        };
        
        static char portstr[32];
        snprintf(portstr, 32, "%u", port);

        if (network->use_tcp_socket) {
                hints.ai_socktype = SOCK_STREAM;
                network->tcp_socket = get_connected_socket(network, host, portstr, &hints);
                if (network->tcp_socket == -1) {
                        return false;
                }
        }

        if (network->use_udp_socket) {
                hints.ai_socktype = SOCK_DGRAM;
                network->udp_socket = get_connected_socket(network, host, portstr, &hints);
                if (network->udp_socket == -1) {
                        return false;
                }
        }

        return true;
}

bool network_run(struct network *network) {
        unsigned i=0;
        if (network->use_tcp_socket) {
                network->fds[i].fd = network->tcp_socket;
                network->fds[i].events = POLLIN;
                if (network->tcpTriggerWrite) {
                        network->fds[i].events |= POLLOUT;
                }
                i++;
        }
        if (network->use_udp_socket) {
                network->fds[i].fd = network->udp_socket;
                network->fds[i].events = POLLIN;
                if (network->udpTriggerWrite) {
                        network->fds[i].events |= POLLOUT;
                }
                i++;
        }

        sigset_t prev;
        if (!disable_all_signals(&prev)) {
                add_syscall_error(network, "pthread_sigmask");
                return false;
        }

        bool ret;
        for (;;) {
                struct pollfd fds[2];
                for (unsigned j=0; j<i; j++) {
                        fds[j] = network->fds[j];
                }
                for (unsigned j=0; j<i; j++) {
                        fds[j].events = network->fds[j].events;
                }
                
                int res = ppoll(fds, i, NULL, &prev);
                if (res == -1) {
                        if (errno == EINTR) {
                                ret = true;
                                break;
                        } else if (errno == EAGAIN) {
                                continue;
                        } else {
                                add_syscall_error(network, "ppoll");
                                ret = false;
                                break;
                        }
                }

                bool must_exit = false;
                for (unsigned j=0; j<i && res>0; j++) {
                        short revents = network->fds[j].revents;
                        int socket = network->fds[j].fd;
                        bool udp = socket == network->udp_socket;
                        
                        if (revents != 0) {
                                res--;
                                if (revents & POLLNVAL) {
                                        add_local_error(network, "invalid socket file descriptor");
                                        must_exit = true;
                                        break;
                                }
                                
                                if (revents & POLLIN || revents & POLLERR || revents & POLLHUP) {
                                        eventBroker_fire(EVENT_BROKER_TCP_RECV+udp, &network->fds[j].fd);
                                }
                                if (revents & POLLOUT) {
                                        eventBroker_fire(EVENT_BROKER_TCP_SEND+udp, &network->fds[j].fd);
                                }
                        }
                }

                if (must_exit) {
                        ret = false;
                        break;
                }
        }

        int err = pthread_sigmask(SIG_SETMASK, &prev, NULL);
        if (err != 0) {
                add_syscall_error(network, "sigprocmask");
                return false;
        }
        return ret;
}
void network_setTriggerOnWriteableTCP(struct network *network, bool value) {
        if (network->use_tcp_socket) {
                network->tcpTriggerWrite = value;
                if (value) {
                        network->fds[0].events |= POLLOUT;
                } else {
                        network->fds[0].events &= ~POLLOUT;
                }
                
        }
}
void network_setTriggerOnWriteableUDP(struct network *network, bool value) {
        if (network->use_udp_socket) {
                network->udpTriggerWrite = value;
                
                int i = 0;
                if (network->use_tcp_socket) {
                        i = 1;
                }
                
                if (value) {
                        network->fds[i].events |= POLLOUT;
                } else {
                        network->fds[i].events &= ~POLLOUT;
                }
        }
}

void network_retrieveError(struct network *network, char *msg, size_t len) {
        static const char *unk_err = "unknown error";
        static const char *no_err = "no error";

        memcpy(msg, no_err, strlen(no_err));
        
        growingArray_foreach_START(&network->errors, struct network_error*, err) {
                char buff[1024];
                const char *str;
                
                switch (err->type) {
                case NETWORK_ERROR_SYSCALL:
                        str = buff;
                        strerror_r(err->code, buff, 1024);
                        break;
                case NETWORK_ERROR_GETADDRINFO:
                        str = gai_strerror(err->code);
                        break;
                case NETWORK_ERROR_LOCAL:
                        str = err->str;
                        break;
                default:
                        str = unk_err;
                }

                size_t str_len = strlen(str);
                if (str_len >= len) {
                        memcpy(msg, str, len-2);
                        msg[len-2] = '\n';
                        msg[len-1] = '\0';
                        return;
                } else {
                        memcpy(msg, str, str_len);
                        len -= str_len;
                        msg += str_len;
                        if (len >= 1) {
                                *msg = '\n';
                                if (len < 2) {
                                        *msg = '\0';
                                        return;
                                } else {
                                        msg++;
                                        *msg = '\0';
                                }
                        }
                }
        } growingArray_foreach_END;
}

void network_destroy(struct network *network) {
        growingArray_destroy(&network->errors);
        if (network->use_tcp_socket && network->tcp_socket != -1) {
                close(network->tcp_socket);
        }
        if (network->use_udp_socket && network->udp_socket != -1) {
                close(network->udp_socket);
        }
}
