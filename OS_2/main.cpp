#include <iostream>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

using namespace std;

volatile sig_atomic_t sighup_received = 0;

void signal_handler(int signo) {
    if (signo == SIGHUP) {
        sighup_received = 1;
    }
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listen_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(listen_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    sigset_t mask, orig_mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
        perror("sigprocmask");
        return 1;
    }

    struct sigaction sa{};
    sa.sa_handler = signal_handler;
    // ИСПРАВЛЕНИЕ ЗДЕСЬ: Добавлен флаг SA_RESTART
    sa.sa_flags = SA_RESTART; 
    sigaction(SIGHUP, &sa, nullptr);

    cout << "Server started on port 8080. PID: " << getpid() << endl;

    int client_fd = -1;
    fd_set read_fds;
    sigset_t empty_mask;
    sigemptyset(&empty_mask);

    while (true) {
        if (sighup_received) {
            cout << "\n[SIGNAL] SIGHUP received!" << endl;
            sighup_received = 0;
        }

        FD_ZERO(&read_fds);
        FD_SET(listen_fd, &read_fds);
        int max_fd = listen_fd;

        if (client_fd != -1) {
            FD_SET(client_fd, &read_fds);
            if (client_fd > max_fd) max_fd = client_fd;
        }

        int res = pselect(max_fd + 1, &read_fds, nullptr, nullptr, nullptr, &empty_mask);

        if (res == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("pselect");
            break;
        }

        if (FD_ISSET(listen_fd, &read_fds)) {
            int new_fd = accept(listen_fd, nullptr, nullptr);
            if (new_fd >= 0) {
                if (client_fd == -1) {
                    client_fd = new_fd;
                    cout << "[NET] New connection accepted. Socket: " << new_fd << endl;
                } else {
                    cout << "[NET] Rejecting extra connection." << endl;
                    close(new_fd);
                }
            }
        }

        if (client_fd != -1 && FD_ISSET(client_fd, &read_fds)) {
            char buffer[1024];
            ssize_t bytes = read(client_fd, buffer, sizeof(buffer));

            if (bytes > 0) {
                cout << "[DATA] Received " << bytes << " bytes." << endl;
            } else {
                cout << "[NET] Connection closed." << endl;
                close(client_fd);
                client_fd = -1;
            }
        }
    }

    close(listen_fd);
    if (client_fd != -1) close(client_fd);

    return 0;
}
