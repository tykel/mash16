Inspector API (mash16)
========================

Overview
--------
The Inspector is an in-process JSON-over-UNIX-socket debug/control interface for mash16. It provides programmatic introspection (registers, memory), control (run/pause/step), breakpoints, event subscription, and snapshot/restore.

Availability
------------
- Build with: cmake -DBUILD_INSPECTOR=ON ...
- Start the emulator with: ./mash16 --inspector-socket /tmp/mash16.sock
- The Inspector listens on the provided Unix domain socket path.

Protocol summary
----------------
- Transport: UNIX domain socket (stream).
- Request/response: simple JSON request with a top-level "method" (string) and optional params. Server replies with a JSON object containing either "result" or "error".
- subscribe: keeps the connection open and streams newline-delimited JSON events after sending an initial {"result":"subscribed"} ack.
- snapshot/restore: snapshot returns a base64 string payload; restore accepts base64 payload.

Supported methods (prototype)
-----------------------------
- getRegisters
  Request: {"method":"getRegisters"}
  Response: {"result":{"pc":NNNN,"sp":NNNN,"r":[...],"flags":{"c":0,"z":0,"o":0,"n":0}}}

- readMemory
  Request: {"method":"readMemory","addr":0x1000,"size":64}
  Response: {"result":[0,1,2,...]}  ; bytes as array of integers

- writeMemory
  Request: {"method":"writeMemory","addr":0x2000,"bytes":[1,2,3,4]}
  Response: {"result":true}

- setBreakpoint / clearBreakpoint / listBreakpoints
  setBreakpoint: {"method":"setBreakpoint","addr":0x3000}
  clearBreakpoint: {"method":"clearBreakpoint","addr":0x3000}
  listBreakpoints -> {"result":[addr1,addr2,...]}

- run / pause / step
  Request: {"method":"run"} -> {"result":true}
  Request: {"method":"pause"}
  Request: {"method":"step"}

- subscribe
  Request: {"method":"subscribe"}
  Response: {"result":"subscribed"}\n
  After the ack, the server streams newline-delimited JSON lines for events such as:
    {"type":"instruction","pc_before":...,"pc_after":...,"bytes":[...]}\n
- snapshot / restore
  snapshot: {"method":"snapshot"} -> {"result":"BASE64..."}
  restore: {"method":"restore","data":"BASE64..."}

Notes & limitations
-------------------
- This is a prototype: JSON parsing is minimal, no authentication, and event streams are newline-delimited JSON.
- Snapshot is a base64 blob of registers + memory; size may be large for full-memory dumps.
- Performance: high-frequency events may be sampled or dropped by subscribers; consider batching for heavy use.

Examples
--------
Example: read registers (shell / netcat style)
  $ printf '{"method":"getRegisters"}' | socat - UNIX-CONNECT:/tmp/mash16.sock

Example: simple JSON-RPC-like exchange (Python)
See examples/inspector_client.py for a ready-to-run Python client with helpers for get_registers, read_memory and subscribe.

Contributing & next steps
-------------------------
- Replace ad-hoc JSON parsing with a proper JSON library (nlohmann::json) and adopt JSON-RPC 2.0 semantics.
- Add authentication or local-only permissioning for TCP sockets if enabled.
- Add documented event types, backpressure, and subscription filters.

