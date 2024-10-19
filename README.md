# CPPServer

A simple HTTP server with JSON API written in C++. Trying to implement a simple cache within the server, something like Redis.

To compile, run `make`.

To start the server, run `./server`

The server currently supports three endpoints:

1. `GET /api/hello` - Returns a hello message
2. `POST /api/echo` - Echoes back the JSON request body
3. `GET /api/cached` - Temporarily simulates an expensive GET operation

To test with curl, run the following on a separate terminal instance after starting the server:

- `GET /api/hello`
  - `curl http://localhost:8080/api/hello`
- `POST /api/echo`
  - `curl -X POST -H "Content-Type: application/json" -d '{"test":"data"}' http://localhost:8080/api/echo`
- `GET /api/cached`
  - `curl http://localhost:8080/api/cached`
  - Expected: `{
    "message": "This response was expensive to compute! :(",
    "cached": false,
    "timestamp": 1697673245000
}`

Simple tests have been added and automated with Github Workflows.
