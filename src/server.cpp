#include "server.hpp"

std::string HttpServer::handle_request(const std::string &request) {
  if (request.find("GET /api/export") != std::string::npos) {
    return export_cache_data();
  } else if (request.find("POST /api/cached") != std::string::npos) {
    size_t body_start = request.find("\r\n\r\n") + 4;
    if (body_start != std::string::npos) {
      try {
        json request_body = json::parse(request.substr(body_start));
        std::string key = request_body["key"].get<std::string>();
        std::string value = request_body["value"].get<std::string>();
        int ttl = request_body.value("ttl", 300);
        cache.put(key, value, std::chrono::seconds(ttl));
        json response = {{"message", "Entry cached successfully"},
                         {"key", key},
                         {"ttl", ttl},
                         {"status", "success"}};
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

  else if (request.find("GET /api/cached/") != std::string::npos) {
    size_t start_pos = request.find("/api/cached/") + 12;
    size_t end_pos = request.find(" HTTP/");

    if (start_pos == std::string::npos || end_pos == std::string::npos ||
        start_pos >= end_pos) {
      json error = {{"error", "Invalid request"}, {"status", "error"}};
      return "HTTP/1.1 400 Bad Request\r\nContent-Type: "
             "application/json\r\n\r\n" +
             error.dump();
    }

    std::string key = request.substr(start_pos, end_pos - start_pos);
    std::string value;

    if (cache.get(key, value)) {
      json response = {{"key", key}, {"value", value}, {"status", "success"}};
      return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" +
             response.dump();
    } else {
      json error = {{"error", "Key not found"}, {"status", "error"}};
      return "HTTP/1.1 404 Not Found\r\nContent-Type: "
             "application/json\r\n\r\n" +
             error.dump();
    }
  }

  else if (request.find("POST /api/cache/clear") != std::string::npos) {
    cache.clear();
    json response = {{"message", "Cache cleared"}, {"status", "success"}};
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" +
           response.dump();
  }

  else if (request.find("GET /api/hello") != std::string::npos) {
    json response = {{"message", "Hello, World!"}, {"status", "success"}};
    return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n" +
           response.dump();
  }

  else if (request.find("POST /api/echo") != std::string::npos) {
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

HttpServer::HttpServer(int port, std::atomic<bool> &stop)
    : port(port), stop_signal(stop), cache(1024, std::chrono::seconds(300)) {};

void HttpServer::start() {
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);
  int addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    throw std::runtime_error("Socket creation failed");
  }

  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    close(server_fd);
    throw std::runtime_error("Setsockopt failed");
  }

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    close(server_fd);
    throw std::runtime_error("Bind failed");
  }

  if (listen(server_fd, 3) < 0) {
    close(server_fd);
    throw std::runtime_error("Listen failed");
  }

  std::cout << "Server listening on port " << port << std::endl;

  fd_set readfds;
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  while (!stop_signal.load()) {
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);

    int activity = select(server_fd + 1, &readfds, NULL, NULL, &tv);

    if (activity < 0) {
      if (errno == EINTR)
        continue;
      break;
    }

    if (activity == 0)
      continue;

    if (FD_ISSET(server_fd, &readfds)) {
      int new_socket =
          accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
      if (new_socket < 0)
        continue;

      char buffer[BUFFER_SIZE] = {0};
      read(new_socket, buffer, BUFFER_SIZE);
      std::string response = handle_request(buffer);
      send(new_socket, response.c_str(), response.length(), 0);
      close(new_socket);
    }
  }
  close(server_fd);
}

std::string HttpServer::export_cache_data() {
  try {
    // Get current timestamp as string
    auto now = std::chrono::system_clock::now();
    auto now_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ts;
    ts << std::put_time(std::localtime(&now_t), "%Y-%m-%d %H:%M:%S");

    // Create the export data JSON
    json export_data = {{"timestamp", ts.str()}, {"entries", json::array()}};

    // Get database connection through cache
    DatabaseConnection *db = cache.get_db();
    if (!db) {
      throw std::runtime_error("Database connection not available");
    }

    pqxx::work txn(*db->get_connection());

    auto result = txn.exec("SELECT key, value, expiry, created_at "
                           "FROM cache_entries "
                           "WHERE expiry > CURRENT_TIMESTAMP");

    for (const auto &row : result) {
      export_data["entries"].push_back(
          {{"key", row[0].as<std::string>()},
           {"value", row[1].as<std::string>()},
           {"expiry", row[2].as<std::string>()},
           {"created_at", row[3].as<std::string>()}});
    }

    txn.commit();

    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: application/json\r\n"
           "Content-Disposition: attachment; "
           "filename=cache_export.json\r\n\r\n" +
           export_data.dump(2);
  } catch (const std::exception &e) {
    json error = {{"error", "Export failed: " + std::string(e.what())},
                  {"status", "error"}};
    return "HTTP/1.1 500 Internal Server Error\r\n"
           "Content-Type: application/json\r\n\r\n" +
           error.dump();
  }
}

HttpServer::~HttpServer() { close(server_fd); }
