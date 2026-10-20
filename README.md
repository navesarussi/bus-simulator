# Bus Simulator

A small C11 systems project: a deterministic bus scheduler with a binary message protocol, plus a POSIX shared-memory channel between processes.

Inspired by deterministic bus messaging and shared-memory races — but **not** a MIL-STD-1553 implementation.

Built with AI assistance; I own the design and can explain every component.

## Components

| Module | Topics |
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

`broken` can report torn reads under concurrent writer/reader processes (timing-dependent; a run may show zero). `fixed` uses a process-shared mutex around the critical section (same class of fix as atomic publication — serialize readers/writers on shared state).

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

## Status

- [x] Message serialize/parse + tests
- [x] Round-robin scheduler + bus dispatch
- [x] Shared-memory record + ring channel
- [x] Race demo (broken vs fixed)
- [x] Throughput benchmark
- [ ] Valgrind CI job
- [ ] Optional: mutex-based alternative to atomics

## License

MIT — see [`LICENSE`](LICENSE).
