# Industrial IoT Telemetry Ingestion & Predictive Analytics Engine

A production-grade, real-world application showcasing the seamless convergence of **all** major language tiers and systems in **xlang**:

1. **Tier 1 Vectorcall Extension SDK**:
   - **SQLite3 Engine (`xsqlite3.so`)**: Schema setup, indices, atomic transactions, analytical queries (`query_int`, `query_float`, `query_value`), and export formats (`query_json`, `query_csv`).
   - **Zlib Compression (`xzlib.so`)**: Stream integrity hashing (`crc32`, `adler32`), in-memory Deflate compression (`compress`, `uncompress`), and direct disk Gzip streaming (`gz_write`, `gz_read`).
2. **Tier 2 Direct C-ABI FFI (Zero-Wrapper System Library Binding)**:
   - **`extern "libm.so"`**: 3D Euclidean vector magnitude (`sqrt`, `pow`) and atmospheric dew point temperature approximation via Magnus-Tetens formulation (`log`, `pow`).
   - **`extern "libc.so.6"`**: String and numerical utilities (`abs`, `atoi`, `atof`).
3. **Tier 3 LLVM AOT Native Compiler Integration**:
   - Single-command compilation to optimized standalone native ELF binaries (`xlang build`).
4. **Object-Oriented Programming (OOP)**:
   - Domain modeling with `SensorDevice()` and `TelemetryReading()` classes, custom constructors, fields, and instance methods.

---

## Architecture Flow

```text
  [Physical Industrial Sensors]
                │
                ▼ (Raw 3D Accelerations, Temp, Humidity, Pressure, Power)
   ┌──────────────────────────┐
   │ OOP Domain Modeling      │
   │  - SensorDevice()        │
   │  - TelemetryReading()    │
   └────────────┬─────────────┘
                │
                ▼ (Calculates 3D Magnitude & Magnus-Tetens Dew Point)
   ┌──────────────────────────┐
   │ Direct C-ABI FFI: libm   │  <-- sqrt(), pow(), log()
   └────────────┬─────────────┘
                │
                ▼ (Atomic Batch Insert: BEGIN TRANSACTION ... COMMIT)
   ┌──────────────────────────┐
   │ Tier 1 SQLite3 Extension │  <-- Relational Tables, Indices, Constraints
   └────────────┬─────────────┘
                │
        ┌───────┴───────┐
        ▼               ▼
   [Analytical Aggs]   [Multi-Format Exports]
    - query_int()       - query_json() (REST APIs)
    - query_float()     - query_csv()  (Data Science)
    - query_value()     └───────┬──────┘
                                │
                                ▼ (Integrity Hashing & In-Memory Deflate)
                       ┌──────────────────────────┐
                       │ Tier 1 Zlib Extension    │  <-- crc32(), adler32(), compress()
                       └────────┬─────────────────┘
                                │
                                ▼ (Disaster Recovery & Snapshots)
                       ┌──────────────────────────┐
                       │ Gzip Compressed Archive  │  <-- gz_write(), gz_read()
                       │ (telemetry_snapshot.gz)  │
                       └──────────────────────────┘
```

---

## How to Run

### Method 1: Bytecode Virtual Machine (VM)
Run directly with fast startup and zero compilation overhead:
```bash
./bin/Release/xlang examples/telemetry_pipeline/main.xb
```

### Method 2: Standalone Native AOT Binary (`xlang build`)
Compile into an optimized, standalone native ELF executable:
```bash
# Build standalone native binary
./bin/Release/xlang build examples/telemetry_pipeline/main.xb -lm -lc -o telemetry_app

# Run the native binary
./telemetry_app
```

---

## Sample Pipeline Output

```text
==================================================================
  Industrial IoT Telemetry Ingestion & Predictive Analytics Engine
==================================================================
Runtime Engines : SQLite3 v3.46.1 | Zlib v1.3.1 | libm & libc C-ABI

--- [Phase 1] Database Initialization & Schema Setup ---
Connected to in-memory telemetry database (handle: 1)
Schema and indices successfully created.

--- [Phase 2] Sensor Device Fleet Registration ---
  [DEV-01] Furnace Core Thermal Probe | Zone: Zone-A | Type: Thermal/Radiation (100 Hz)
  [DEV-02] Turbine Shaft 1 Main Bearing | Zone: Zone-B | Type: Vibration/Triaxial (1000 Hz)
  [DEV-03] Cleanroom Climate Sensor | Zone: Zone-C | Type: Environmental/RH (10 Hz)
  [DEV-04] Exhaust Duct Extraction Fan | Zone: Zone-A | Type: Thermal/Airflow (50 Hz)
  [DEV-05] High-Pressure Hydraulic Pump | Zone: Zone-B | Type: Pressure/Vibration (250 Hz)
Registered 5 sensor devices in fleet.

--- [Phase 3] Batch Telemetry Ingestion & Scientific Computation ---
Ingestion complete: 22 records stored in database.

--- [Phase 4] Real-Time Analytical Aggregations ---
Fleet Average Temperature  : 50.14 C
Peak Vibration Observed    : 13.640 mm/s^2
Device with Peak Vibration : Turbine Shaft 1 Main Bearing
Total Energy Consumption   : 95.20 kW
Critical Safety Alarms     : 2 event(s)

--- [Phase 5] Tabular Zone Summary Export (CSV) ---
zone,readings,avg_temp,max_vib,energy_kw
Zone-A,9,60.4,2.51,23.7
Zone-B,9,52.7,13.64,69.7
Zone-C,4,21.3,0.09,1.8

--- [Phase 6] Incident Triage Payload (JSON) ---
[{"id": 21, "device_id": "DEV-02", "name": "Turbine Shaft 1 Main Bearing", "zone": "Zone-B", "temp_c": 78.4, "vib_mag": 13.64, "dew_c": 58.6}, {"id": 22, "device_id": "DEV-01", "name": "Furnace Core Thermal Probe", "zone": "Zone-A", "temp_c": 94.8, "vib_mag": 2.08, "dew_c": 46.1}]

--- [Phase 7] Data Stream Integrity & Checksum Verification ---
Incident Payload Size : 281 bytes
IEEE 802.3 CRC-32     : 1670724571 (0x63953BDB)
Adler-32 Checksum     : -1637986605 (0x9E5E4ED3)

--- [Phase 8] In-Memory Deflate Compression ---
Upper Bound Estimation : 294 bytes
Compressed Raw Size    : 179 bytes (encoded as 358 hex characters)
Integrity Check        : PASSED (Decompressed string matches original)

--- [Phase 9] Persistent Snapshot Archival & Recovery ---
Serialized snapshot to : 'telemetry_incident_snapshot.json.gz' (281 uncompressed bytes written)
Recovered snapshot from: 'telemetry_incident_snapshot.json.gz' (281 bytes restored)
Recovered CRC-32       : 1670724571
Archival validation    : PASSED (Temporary archive file cleanly removed)

--- [Phase 10] Clean Teardown ---
SQLite Engine Memory Used Before Exit : 98424 bytes
Telemetry database disconnected cleanly.

==================================================================
  Industrial IoT Telemetry Pipeline Run Complete (All Tests PASS)  
==================================================================
```
