# CPPServer

A simple HTTP server with JSON API written in C++. Implements a Redis-like LRU cache with TTL eviction policy. Features include Prometheus monitoring, Grafana dashboards, and Swagger API documentation. Tests are written with Google Tests and automated with Github Workflows.

This project is not intended to be highly robust or suitable for consistent use; rather, it serves as a way to gain exposure to various open-source technologies.

### Prerequisites

Two ways to run this project:

#### 1. Docker (Recommended)
- Docker
- Docker Compose

#### 2. Local Setup (MacOS)
- Homebrew for dependency management
- Dependencies:
```bash
brew install nlohmann-json              # JSON for modern C++
brew install googletest                 # Testing framework
brew install prometheus prometheus-cpp  # Monitoring system with C++ client
brew install postgresql@14 libpqxx      # PostgreSQL database with C++ client
```

### Setup

#### Docker Setup (Recommended)
```bash
# Build and start all services
docker-compose -f docker/docker-compose.yml up --build

# Stop all services
docker-compose -f docker/docker-compose.yml down

# Remove all data (including database)
docker-compose -f docker/docker-compose.yml down -v
```

#### Local Setup
1. Install dependencies: `make prometheus_deps`
2. Start PostgreSQL: `brew services start postgresql@14`
3. Create database: `createdb cache_db`
4. Create PostgreSQL user: `createuser --superuser $(whoami)`
5. Build the project: `make all`
6. Start the server: `./server`
7. Start Prometheus: `prometheus --config.file=./prometheus.yml`

Services will be available at:
- API Server: http://localhost:8080
- Swagger UI: http://localhost:8081
- Prometheus: http://localhost:9090
- Grafana: http://localhost:3000
- PostgreSQL: localhost:5432

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

#### Grafana Dashboard
Access the monitoring dashboard at http://localhost:3000
Features:
- Cache Performance Graph (hits/misses)
- Current Cache Size
- Auto-refresh every 5 seconds
- Historical data view

#### Prometheus Metrics
Access raw metrics at http://localhost:9090
Available metrics:
- `cache_hits_total`: Cache hit count
- `cache_misses_total`: Cache miss count
- `cache_evictions_total`: Number of evicted items
- `cache_expired_total`: Number of expired items
- `cache_size_bytes`: Current cache size
- `cache_memory_usage_bytes`: Memory usage

### API Documentation

Access the Swagger UI at http://localhost:8081
Features:
- Interactive API documentation
- Try out endpoints directly in the browser
- Request/response examples
- Download OpenAPI specification

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

### Docker Services Overview

The project uses several Docker containers:
- `app`: The main C++ server application
- `db`: PostgreSQL database for persistence
- `prometheus`: Metrics collection
- `grafana`: Metrics visualization
- `swagger-ui`: API documentation

Configuration files:
- `docker/docker-compose.yml`: Service definitions
- `docker/Dockerfile`: C++ application build
- `prometheus.yml`: Prometheus configuration
- `docker/grafana/`: Grafana dashboards and configuration
- `docs/api/openapi.yml`: API specification

Thanks for checking out this project! :)
