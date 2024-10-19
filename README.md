# CPPServer

A simple HTTP server with JSON API written in C++.

To compile, run `make`.

To start the server, run `./server`

The server currently supports two endpoints:
1. GET /api/hello - Returns a hello message
2. POST /api/echo - Echoes back the JSON request body

To test with curl, run the following on a separate terminal instance after starting the server:
- `curl http://localhost:8080/api/hello`
- `curl -X POST -H "Content-Type: application/json" -d '{"test":"data"}' http://localhost:8080/api/echo`

Tests have been added with github workflows.
