# CPPServer

A simple HTTP server with JSON API written in C++. Trying to implement a simple cache within the server, something like Redis.

To compile, run `make`.

To start the server, run `./server`

The server currently supports three endpoints:

1. `GET /api/hello` - Returns a hello message
2. `POST /api/echo` - Echoes back the JSON request body
3. `GET /api/cached` - Simulates an expensive GET operation (WIP)
4. `GET /api/cache/stats` - Access cache statistics
5. `POST /api/cache/clear` - Clear cache

You can test with `curl` by running the following on a separate terminal instance after starting the server:

- `GET /api/hello`
  - `curl http://localhost:8080/api/hello`
  - Expected: `{"message":"Hello, World!","status":"success"}`
- `POST /api/echo`
  - `curl -X POST -H "Content-Type: application/json" -d '{"test":"data"}' http://localhost:8080/api/echo`
  - Expected: `{"echo":{"test":"data"},"status":"success"}`
- `GET /api/cached`
  - `curl http://localhost:8080/api/cached`
  - Expected: `{"message": "This response was expensive to compute! :(", "cached": false, "timestamp": 1697673245000}`
- `GET /api/cache/stats`
  - `curl http://localhost:8080/api/cache/stats`
  - Expected: `{"cache_capacity":1024,"cache_size":1}`
- `POST /api/cache/clear`
  - `curl -X POST http://localhost:8080/api/cache/clear`
  - Expected: `{"message":"Cache cleared","status":"success"}`

Simple tests have been added and automated with Github Workflows.
