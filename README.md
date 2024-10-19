# CPPServer

A simple HTTP server with JSON API written in C++. Trying to implement a simple LRU cache with TTL eviction policy, something similar to Redis.

To compile, run `make`.

To start the server, run `./server`, then `curl` with in separate terminal instances.

The server currently supports six endpoints:

1. `GET /api/hello` - Returns a hello message
2. `POST /api/echo` - Echoes back the JSON request body
3. `POST /api/cached` - Caches a JSON entry into the server cache
4. `GET /api/cached` - Attempts to access a key and return its data
5. `GET /api/cache/stats` - Access cache statistics
6. `POST /api/cache/clear` - Clear cache

Here are some `curl` commands you can run after starting the server to try this server cache:

- `GET /api/hello`
  - `curl http://localhost:8080/api/hello`
  - Expected: `{"message":"Hello, World!","status":"success"}`
- `POST /api/echo`
  - `curl -X POST -H "Content-Type: application/json" -d '{"test":"data"}' http://localhost:8080/api/echo`
  - Expected: `{"echo":{"test":"data"},"status":"success"}`
- `POST /api/cached`
  - `curl -X POST http://localhost:8080/api/cached -H "Content-Type: application/json" -d '{"key": "user123", "value": "John Doe", "ttl": 60}`
  - Expected: `{"key":"user123","message":"Entry cached successfully","status":"success","ttl":60}`
- `GET /api/cached`
  - `curl http://localhost:8080/api/cached/user123`
  - Expected: `{"key":"user123","status":"success","value":"John Doe"}`
- `GET /api/cache/stats`
  - `curl http://localhost:8080/api/cache/stats`
  - Expected: `{"cache_capacity":1024,"cache_size":1}`
- `POST /api/cache/clear`
  - `curl -X POST http://localhost:8080/api/cache/clear`
  - Expected: `{"message":"Cache cleared","status":"success"}`

Simple tests have been added and automated with Github Workflows.
