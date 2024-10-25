# CPPServer

A simple HTTP server with JSON API written in C++. Trying to implement a simple LRU cache with TTL eviction policy, something similar to Redis. Prometheus is also implemented for simple monitoring. Simple testing has also been automated with Github Workflows.

### Prerequisites

This project was developed on MacOS and requires Homebrew for dependency management.

Required dependencies:
```bash
brew install nlohmann-json              # JSON for modern C++
brew install googletest                 # Testing framework
brew install prometheus prometheus-cpp  # Monitoring system with C++ client
brew install postgresql@14 libpqxx      # PostgreSQL database with C++ client
```

### Setup

1. Install dependencies: `make prometheus_deps`
2. Start PostgreSQL: `brew services start postgresql@14`
3. Create database: `createdb cache_db`
4. Create PostgreSQL user: `createuser --superuser $(whoami)`
5. Build the project: `make all`
6. Start the server: `./server`
7. Start Prometheus: `prometheus --config.file=./prometheus.yml`

The server runs on `localhost:8080`, Prometheus on `localhost:9090`, and PostgreSQL on `localhost:5432`.

### API Endpoints

1. `GET /api/hello` - Health check endpoint
2. `POST /api/echo` - Echo service for testing
3. `POST /api/cached` - Store data in cache with TTL
4. `GET /api/cached/{key}` - Retrieved cached data
5. `POST /api/cache/clear` - Clear cache
6. `GET /api/export` - Export current cache state to JSON file

### Usage Examples

```bash
# Health check
curl http://localhost:8080/api/hello
# Expected: {"message":"Hello, World!","status":"success"}

# Store data in cache
curl -X POST http://localhost:8080/api/cached \
  -H "Content-Type: application/json" \
  -d '{
    "key": "user123",
    "value": "John Doe",
    "ttl": 3600
  }'
# Expected: {"key":"user123","message":"Entry cached successfully","status":"success","ttl":3600}

# Retrieve cached data
curl http://localhost:8080/api/cached/user123
# Expected: {"key":"user123","status":"success","value":"John Doe"}

# Export cache state
curl -O -J "http://localhost:8080/api/export"
# Downloads cache_export.json with current cache state

# Clear cache
curl -X POST http://localhost:8080/api/cache/clear
# Expected: {"message":"Cache cleared","status":"success"}
```

### Data Persistence

This system aims to utilise a two-tier storage approach:

1. In-memory LRU cache for fast access
2. PostgreSQL database for persistence

Data is automatically synchronised between tiers:

- Writes go to both memory and database
- Cache misses check the database
- TTL expiration is handled in both tiers

### Monitoring

1. Open `http://localhost:9090/`
2. Available metrics:
   - `cache_hits_total`: Cache hit count
   - `cache_misses_total`: Cache miss count
   - `cache_evictions_total`: Number of evicted items
   - `cache_expired_total`: Number of expired items
   - `cache_size_bytes`: Current cache size
   - `cache_memory_usage_bytes`: Memory usage

To view metrics:
1. Enter metric name in query box
2. Click "Execute"
3. Switch between "Table" and "Graph" views
4. Refresh after cache operations to see updates

### Database Management

View cache entries directly in PostgreSQL:
```bash
# Connect to database
psql cache_db

# View all cache entries
SELECT * FROM cache_entries;

# View non-expired entries
SELECT * FROM cache_entries WHERE expiry > CURRENT_TIMESTAMP;

# Exit
\q
```

Thanks for checking out this project! :)
