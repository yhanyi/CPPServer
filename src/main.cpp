#include "server.hpp"

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
