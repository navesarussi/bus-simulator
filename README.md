# Bus Simulator

Educational C project for **embedded/avionics** and **platform / high-performance systems** interview prep.

Inspired by deterministic bus messaging and production debugging of shared-memory races — but **not** a MIL-STD-1553 implementation.

## Who this is for

| Target role | Examples | What this repo demonstrates |
|-------------|----------|-----------------------------|
| Embedded / avionics | real-time firmware, device software | fixed layouts, checksums, deterministic scheduler |
| Platform / systems | Vast Data, Check Point, storage, security | POSIX shared memory, atomics, torn-read detection, benchmarks |

## Components

| Module | Interview topics |
|--------|------------------|
| `message` | binary protocols, endianness, validation, static allocation |
| `scheduler` + `bus` | deterministic scheduling, fairness, state machines |
| `shm_channel` | `mmap`/`shm_open`, release/acquire atomics, ring buffer |
| `tools/race_demo` | data races, torn reads, before/after fix |
| `tools/bench_shm` | throughput measurement, hot-path awareness |

## Build

```bash
make test          # unit tests
make tools         # race demo + benchmark binaries
make demo-race     # broken vs fixed race comparison
make bench         # single-process ring throughput
```

### Race demo

```bash
./build/race_demo 200000 both
./build/race_demo 200000 broken
./build/race_demo 200000 fixed
```

Expect `broken` to report torn reads under concurrent writer/reader processes. `fixed` uses a process-shared mutex around the critical section (same class of fix as atomic publication — serialize readers/writers on shared state).

## Learning map (8 weeks)

1. Message format + parser
2. Scheduler + bus dispatch
3. Shared memory + atomics
4. Race demo + benchmark
5. Valgrind + CI hardening
6. README deep-dive + interview stories
7. LeetCode pointers/bits
8. Apply + iterate from real interviews

## Message layout (big-endian on the wire)

| Field | Size |
|-------|------|
| sync | 2 bytes (`0xA5A5`) |
| RT address | 1 byte (0-31) |
| subaddress | 1 byte (0-31) |
| word count | 1 byte (0-32) |
| flags | 1 byte |
| data words | `word_count * 2` bytes |
| checksum | 2 bytes |

## Interview pitch (30 seconds)

> I built a small C simulator that combines a deterministic bus scheduler with a POSIX shared-memory channel. One module intentionally reproduces torn reads under concurrent processes; the fixed path uses atomic release/acquire on a sequence word — similar to a production race I debugged on shared memory between processes.

## Status

- [x] Message serialize/parse + tests
- [x] Round-robin scheduler + bus dispatch
- [x] Shared-memory record + ring channel
- [x] Race demo (broken vs fixed)
- [x] Throughput benchmark
- [ ] Valgrind CI job
- [ ] Optional: mutex-based alternative to atomics
