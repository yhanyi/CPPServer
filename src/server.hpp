#ifndef SERVER_HPP
#define SERVER_HPP

#include "cache.hpp"
#include "database.hpp"
#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <netinet/in.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

using json = nlohmann::json;

class HttpServer {
private:
  int server_fd;
  int port;
  std::atomic<bool> &stop_signal;
  static const int BUFFER_SIZE = 1024;
  LRUCache<std::string, std::string> cache;

  std::string handle_request(const std::string &request);
  std::string export_cache_data();

public:
  HttpServer(int port = 8080,
             std::atomic<bool> &stop = *new std::atomic<bool>(false));
  void start();
  ~HttpServer();
};

#endif
