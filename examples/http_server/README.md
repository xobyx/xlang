# xlang HTTP Web Server Example

A modular, production-ready HTTP web server built with **xlang** featuring RFC-compliant HTTP/1.1 parsing, REST API routing, connection management, automated verification test suites, and micro-benchmarking.

---

## Architecture Overview

```
examples/http_server/
├── http.xb            # HttpRequest & HttpResponse protocol models
├── server.xb          # HttpServer engine (TCP socket, routing, REST state)
├── server_daemon.xb   # Persistent interactive server daemon (listening on :8080)
├── main.xb            # Automated test suite, integration tests & benchmark
└── README.md          # Architecture, endpoints, and usage guide
```

### Module Breakdown

1. **`http.xb`**:
   - `HttpRequest`: Parses raw HTTP wire requests into structured components: HTTP method, request path, raw query string, URL parameters map, headers map (with case-insensitive lookups), and body payload.
   - `HttpResponse`: Status codes, standard headers (`Content-Type`, `Content-Length`, `Server`, `Connection`), helper builders (`.html()`, `.json()`, `.text()`), and serialization to wire format (`.to_string()`).

2. **`server.xb`**:
   - `HttpServer`: Encapsulates TCP socket creation, `SO_REUSEADDR` configuration, address binding, listening, and client connection handling with timeout protection.
   - Built-in dynamic router supporting HTML landing pages, JSON API endpoints, REST collection state, query parameter echo, and 404 fallback.

3. **`main.xb`**:
   - Automated self-testing client that initializes the server on port `18080` via loopback TCP.
   - Executes verification assertions for all endpoints.
   - Runs a 100-request sequential micro-benchmark reporting requests/sec throughput and garbage collector memory metrics.

4. **`server_daemon.xb`**:
   - Runs continuously on port `8080` to allow live interaction from web browsers, `curl`, or API tools.

---

## API Endpoints

| Method | Path | Description | Response Content-Type |
| :--- | :--- | :--- | :--- |
| `GET` | `/` | Web dashboard & endpoint catalog | `text/html; charset=utf-8` |
| `GET` | `/health` | Server health probe & uptime counter | `application/json; charset=utf-8` |
| `GET` | `/api/info` | Runtime diagnostics & heap memory stats | `application/json; charset=utf-8` |
| `GET` | `/api/items` | List in-memory catalog items | `application/json; charset=utf-8` |
| `POST` | `/api/items` | Append a new JSON item to catalog | `application/json; charset=utf-8` (201 Created) |
| `GET` | `/api/echo` | Echo request headers and query parameters | `application/json; charset=utf-8` |
| `*` | `*` | Fallback 404 handler | `application/json; charset=utf-8` (404 Not Found) |

---

## Quick Start & Usage

### 1. Run Automated Test Suite & Benchmark

Execute `main.xb` to run automated assertions and throughput benchmarks:

```bash
./bin/Release/xlang examples/http_server/main.xb
```

Sample output:
```
==================================================
     xlang Production Demo: HTTP Web Server       
  Native Micro-Framework, REST API & Benchmarks   
==================================================
[1] HTTP Server successfully bound to http://127.0.0.1:18080
    Listening on TCP backlog 32 with SO_REUSEADDR enabled.

[2] Executing Automated HTTP Test Suite...
    --> Testing GET / (HTML Landing Page)...
        [PASS] Status: 200 OK | HTML Dashboard correctly rendered
    --> Testing GET /health (JSON Health Check)...
        [PASS] Status: 200 OK | Health payload: status=UP
    --> Testing GET /api/info (System Diagnostics)...
        [PASS] Status: 200 OK | Runtime diagnostics reported
    --> Testing POST /api/items (Create Catalog Items)...
        [PASS] Status: 201 Created | Successfully created 3 catalog items
    --> Testing GET /api/items (List Catalog Items)...
        [PASS] Status: 200 OK | Returned 3 items in JSON array
    --> Testing GET /api/echo?agent=antigravity&version=2.0 (Echo)...
        [PASS] Status: 200 OK | Successfully extracted query params and headers
    --> Testing GET /non_existent_route (404 Fallback)...
        [PASS] Status: 404 Not Found | Correctly rejected unknown endpoint

[3] High-Throughput HTTP Micro-Benchmark (100 sequential requests)...
    Processed 100 full HTTP Request/Response cycles in 21 ms (~4761 req/sec)

[4] Server Diagnostics & Heap Health:
    Total requests served:    109
    Catalog items stored:     3
    GC Active Heap Objects:   521 objects
    GC Allocated Heap Memory: 246799 bytes

[5] Server socket closed. HTTP demonstration finished successfully!
==================================================
```

### 2. Run the Interactive Server Daemon

To start the server and interact with it from your browser or `curl`:

```bash
./bin/Release/xlang examples/http_server/server_daemon.xb
```

In another terminal, test with `curl`:

```bash
# 1. HTML landing page
curl -i http://127.0.0.1:8080/

# 2. JSON health probe
curl -i http://127.0.0.1:8080/health

# 3. Server runtime & GC diagnostics
curl -i http://127.0.0.1:8080/api/info

# 4. Create an item via POST
curl -i -X POST -d '{"name":"Mechanical Keyboard","price":129}' http://127.0.0.1:8080/api/items

# 5. List items via GET
curl -i http://127.0.0.1:8080/api/items

# 6. Echo query parameters and headers
curl -i 'http://127.0.0.1:8080/api/echo?agent=antigravity&version=2.0'

# 7. Non-existent route (404)
curl -i http://127.0.0.1:8080/unknown
```
