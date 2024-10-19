#ifndef SERVER_HPP
#define SERVER_HPP

#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

using json = nlohmann::json;

class HttpServer {
private:
  int server_fd;
  int port;
  static const int BUFFER_SIZE = 1024;

  std::string handle_request(const std::string &request) {
    if (request.find("GET /api/hello") != std::string::npos) {
      json response = {{"message", "Hello, World!"}, {"status", "success"}};
      return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" +
             response.dump();
    } else if (request.find("POST /api/echo") != std::string::npos) {
      // Extract JSON body
      size_t body_start = request.find("\r\n\r\n") + 4;
      if (body_start != std::string::npos) {
        try {
          json request_body = json::parse(request.substr(body_start));
          json response = {{"echo", request_body}, {"status", "success"}};
          return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" +
                 response.dump();
        } catch (const json::parse_error &e) {
          json error = {{"error", "Invalid JSON"}, {"status", "error"}};
          return "HTTP/1.1 400 Bad Request\r\nContent-Type: "
                 "application/json\r\n\r\n" +
                 error.dump();
        }
      }
    }

    json error = {{"error", "Not Found"}, {"status", "error"}};
    return "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n" +
           error.dump();
  }

public:
  HttpServer(int port = 8080) : port(port) {}

  void start() {
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      throw std::runtime_error("Socket creation failed");
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
      throw std::runtime_error("Setsockopt failed");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
      throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd, 3) < 0) {
      throw std::runtime_error("Listen failed");
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (true) {
      int new_socket;
      if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                               (socklen_t *)&addrlen)) < 0) {
        throw std::runtime_error("Accept failed");
      }

      char buffer[BUFFER_SIZE] = {0};
      read(new_socket, buffer, BUFFER_SIZE);

      std::string response = handle_request(buffer);
      send(new_socket, response.c_str(), response.length(), 0);
      close(new_socket);
    }
  }

  ~HttpServer() { close(server_fd); }
};

#ifdef MAIN
int main() {
  try {
    HttpServer server(8080);
    server.start();
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
#endif

#endif
