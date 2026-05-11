#!/usr/bin/env python3
"""Minimal MCP stdio wrapper for the mash16 inspector socket."""

import argparse
import json
import socket
import sys


def inspector_call(path, method, params=None):
    req = {
        "jsonrpc": "2.0",
        "id": 1,
        "method": method,
        "params": params or {},
    }
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as sock:
        sock.connect(path)
        sock.sendall((json.dumps(req) + "\n").encode("utf-8"))
        sock.shutdown(socket.SHUT_WR)
        data = b""
        while True:
            chunk = sock.recv(65536)
            if not chunk:
                break
            data += chunk
    return json.loads(data.decode("utf-8"))


TOOLS = [
    {
        "name": "debug_state",
        "description": "Return registers, stop reason, current instruction, and nearby disassembly.",
        "inputSchema": {"type": "object", "properties": {}},
    },
    {
        "name": "step",
        "description": "Step one or more instructions and return debugger state.",
        "inputSchema": {
            "type": "object",
            "properties": {"count": {"type": "integer", "minimum": 1}},
        },
    },
    {
        "name": "run_until",
        "description": "Resume until an address or symbol is reached, with a timeout.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "target": {"type": "string"},
                "timeoutMs": {"type": "integer", "minimum": 1},
            },
            "required": ["target"],
        },
    },
    {
        "name": "read_memory",
        "description": "Read bytes from Chip16 memory.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "addr": {"type": "integer", "minimum": 0, "maximum": 65535},
                "size": {"type": "integer", "minimum": 1},
            },
            "required": ["addr", "size"],
        },
    },
    {
        "name": "disassemble",
        "description": "Disassemble instructions at an address, symbol, or pc.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "target": {"type": "string"},
                "count": {"type": "integer", "minimum": 1},
            },
        },
    },
    {
        "name": "press_controller_button",
        "description": "Inject a controller button press.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "button": {
                    "type": "string",
                    "enum": ["up", "down", "left", "right", "select", "start", "a", "b"],
                },
                "player": {"type": "integer", "minimum": 1, "maximum": 2},
            },
            "required": ["button"],
        },
    },
    {
        "name": "release_controller_button",
        "description": "Inject a controller button release.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "button": {
                    "type": "string",
                    "enum": ["up", "down", "left", "right", "select", "start", "a", "b"],
                },
                "player": {"type": "integer", "minimum": 1, "maximum": 2},
            },
            "required": ["button"],
        },
    },
]


def tool_call(path, name, args):
    if name == "debug_state":
        return inspector_call(path, "state")
    if name == "step":
        return inspector_call(path, "step", {"count": args.get("count", 1)})
    if name == "run_until":
        return inspector_call(
            path,
            "runUntil",
            {"target": args["target"], "timeoutMs": args.get("timeoutMs", 5000)},
        )
    if name == "read_memory":
        return inspector_call(path, "readMemory", {"addr": args["addr"], "size": args["size"]})
    if name == "disassemble":
        return inspector_call(
            path,
            "disassemble",
            {"target": args.get("target", "pc"), "count": args.get("count", 8)},
        )
    if name == "press_controller_button":
        return inspector_call(
            path,
            "pressControllerButton",
            {"button": args["button"], "player": args.get("player", 1)},
        )
    if name == "release_controller_button":
        return inspector_call(
            path,
            "releaseControllerButton",
            {"button": args["button"], "player": args.get("player", 1)},
        )
    raise ValueError(f"unknown tool: {name}")


def send(message):
    sys.stdout.write(json.dumps(message, separators=(",", ":")) + "\n")
    sys.stdout.flush()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", default="/tmp/mash16.sock")
    args = parser.parse_args()

    for line in sys.stdin:
        if not line.strip():
            continue
        req = json.loads(line)
        req_id = req.get("id")
        method = req.get("method")
        try:
            if method == "initialize":
                result = {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {"tools": {}},
                    "serverInfo": {"name": "mash16-inspector", "version": "1"},
                }
            elif method == "tools/list":
                result = {"tools": TOOLS}
            elif method == "tools/call":
                params = req.get("params", {})
                payload = tool_call(args.socket, params["name"], params.get("arguments", {}))
                result = {
                    "content": [
                        {
                            "type": "text",
                            "text": json.dumps(payload, indent=2),
                        }
                    ]
                }
            elif method == "notifications/initialized":
                continue
            else:
                raise ValueError(f"unknown method: {method}")
            send({"jsonrpc": "2.0", "id": req_id, "result": result})
        except Exception as exc:
            send(
                {
                    "jsonrpc": "2.0",
                    "id": req_id,
                    "error": {"code": -32000, "message": str(exc)},
                }
            )


if __name__ == "__main__":
    main()
