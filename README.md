# CPPServer

A simple HTTP server with JSON API written in C++. Trying to implement a simple LRU cache with TTL eviction policy, something similar to Redis. Prometheus is also implemented for simple monitoring. Simple testing has also been automated with Github Workflows.

### Prerequisites

This project was made on MacOS so it assumes setup on MacOS. Homebrew should also be installed.

Run the following to install dependencies:

```
brew install nlohmann-json
brew install googletest
brew install prometheus prometheus-cpp
```

### Setup

1. Install dependencies: `make prometheus_deps`
2. Build the project: `make all`
3. Start the server: `./server`
4. Start Prometheus: `prometheus --config.file=./prometheus.yml`

The server will run on `localhost:8080` and Prometheus will run on `localhost:9090`.

### Usage

The server currently supports six endpoints:

1. `GET /api/hello` - Returns a hello message
2. `POST /api/echo` - Echoes back the JSON request body
3. `POST /api/cached` - Caches a JSON entry into the server cache
4. `GET /api/cached` - Attempts to access a key and return its data
5. `GET /api/cache/stats` - Access cache statistics
6. `POST /api/cache/clear` - Clear cache

Here are some `curl` commands you can run after starting the server to try the server cache:

- `GET /api/hello`
  - `curl http://localhost:8080/api/hello`
  - Expected: `{"message":"Hello, World!","status":"success"}`
- `POST /api/echo`
  - `curl -X POST -H "Content-Type: application/json" -d '{"test":"data"}' http://localhost:8080/api/echo`
  - Expected: `{"echo":{"test":"data"},"status":"success"}`
- `POST /api/cached`
  - `curl -X POST http://localhost:8080/api/cached -H "Content-Type: application/json" -d '{"key": "user123", "value": "John Doe", "ttl": 60}'`
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

### Prometheus

To access monitoring of cache operations,

1. Open `http://localhost:9090/` in your browser
2. Type "cache" in the query box
3. Click "Execute"
4. Switch between "Table" and "Graph" views
5. Refresh the UI / Click "Execute" when you perform a cache operation with `curl`

Within the query box you should see suggested metrics like `cache_hits_total`, `cache_misses_total`, `cache_size_bytes`, etc

Thanks for checking out this project! :)
