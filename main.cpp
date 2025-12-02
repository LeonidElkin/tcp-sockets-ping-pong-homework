/*
 * This program demonstrates inter-process synchronization using TCP sockets
 * on a UNIX-like system. The application creates two processes via fork():
 *
 *   • Process A acts as a TCP server. It starts in the READY state, performs
 *     some simulated work, sends a "PING" message to Process B, and then
 *     switches to the SLEEP state while waiting for a reply.
 *
 *   • Process B acts as a TCP client. It starts in the SLEEP state, connects
 *     to Process A, waits for the "PING" message, switches to READY, performs
 *     its own simulated work, and replies with "PONG".
 *
 * The two processes exchange these messages in a ping-pong pattern for a
 * fixed number of iterations. This demonstrates how a reliable full-duplex
 * TCP connection can be used as a synchronization primitive between separate
 * processes, ensuring ordered message delivery and deterministic turn-taking.
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <format>

using namespace std::chrono_literals;

constexpr int PORT = 9889;
constexpr int MAX_ITERATIONS = 6;

[[noreturn]] void sys_error(const std::string &msg) {
	throw std::runtime_error(std::format("{}: {}", std::string(msg), std::strerror(errno)));
}

class Socket {
public:
	Socket() = default;

	explicit Socket(int fd) : fd_(fd) {
		if (fd_ < 0) sys_error("socket");
	}

	~Socket() {
		if (fd_ >= 0) close(fd_);
	}

	Socket(const Socket &) = delete;

	Socket &operator=(const Socket &) = delete;

	Socket(Socket &&other) noexcept: fd_(other.fd_) {
		other.fd_ = -1;
	}

	Socket &operator=(Socket &&other) noexcept {
		if (this != &other) {
			if (fd_ >= 0) close(fd_);
			fd_ = other.fd_;
			other.fd_ = -1;
		}
		return *this;
	}

	[[nodiscard]] int fd() const { return fd_; }

	void send_str(std::string_view s) const {
		if (send(fd_, s.data(), s.size() + 1, 0) < 0)
			sys_error("send");
	}

	[[nodiscard]] std::string recv_str() const {
		char buf[128];
		ssize_t n = recv(fd_, buf, sizeof(buf), 0);
		if (n <= 0) sys_error("recv");

		return {buf};
	}

private:
	int fd_ = -1;
};

class TcpServer {
public:
	TcpServer() {
		listen_fd_ = Socket(socket(AF_INET, SOCK_STREAM, 0));

		int opt = 1;
		if (setsockopt(listen_fd_.fd(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
			sys_error("setsockopt");

		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(PORT);

		if (bind(listen_fd_.fd(), reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
			sys_error("bind");

		if (listen(listen_fd_.fd(), 1) < 0)
			sys_error("listen");
	}

	[[nodiscard]] Socket accept_client() const {
		sockaddr_in caddr{};
		socklen_t len = sizeof(caddr);
		int cfd = accept(listen_fd_.fd(), reinterpret_cast<sockaddr *>(&caddr), &len);
		if (cfd < 0) sys_error("accept");
		return Socket(cfd);
	}

private:
	Socket listen_fd_;
};

void process_a(Socket conn) {
	std::cout << "[Process A] Initial state: READY\n";

	for (int i = 0; i < MAX_ITERATIONS; ++i) {
		std::cout << "\n--- Iteration " << (i + 1) << " (A) ---\n";

		// simulate work
		std::this_thread::sleep_for(1s);

		std::cout << "[A] Sending PING...\n";
		conn.send_str("PING");

		std::cout << "[A] Waiting for response...\n";
		std::string msg = conn.recv_str();
		std::cout << "[A] Received: " << msg << ", entering READY\n";
	}

	std::cout << "[Process A] Finished.\n";
}

void process_b() {
	// Create client socket
	Socket sock(socket(AF_INET, SOCK_STREAM, 0));

	sockaddr_in serv{};
	serv.sin_family = AF_INET;
	serv.sin_port = htons(PORT);

	if (inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr) <= 0)
		sys_error("inet_pton");

	// give the server some time to become ready
	sleep(1);

	if (connect(sock.fd(), reinterpret_cast<sockaddr *>(&serv), sizeof(serv)) < 0)
		sys_error("connect");

	std::cout << "[Process B] Initial state: SLEEP\n";

	for (int i = 0; i < MAX_ITERATIONS; ++i) {
		std::cout << "\n--- Iteration " << (i + 1) << " (B) ---\n";

		std::cout << "[B] Waiting for PING...\n";
		std::string msg = sock.recv_str();
		std::cout << "[B] Received: " << msg << ", entering READY\n";

		// simulate work
		std::this_thread::sleep_for(1s);

		std::cout << "[B] Sending PONG...\n";
		sock.send_str("PONG");
	}

	std::cout << "[Process B] Finished.\n";
}

// ----------------------- main ---------------------------------------------

int main() {
	try {
		std::cout << "=== TCP Ping-Pong ===\n";

		TcpServer server;  // server socket created and listening here

		pid_t pid = fork();
		if (pid < 0) sys_error("fork");

		if (pid == 0) {
			// child → Process B (client)
			process_b();
			_exit(0);
		}

		// parent → Process A (server side)
		Socket conn = server.accept_client();
		process_a(std::move(conn));

		int status = 0;
		waitpid(pid, &status, 0);

		std::cout << "\n=== Done ===\n";
		return 0;

	} catch (const std::exception &ex) {
		std::cerr << "Fatal error: " << ex.what() << "\n";
		return 1;
	}
}
