#!/usr/bin/env python3
"""
Simple Inspector client examples for mash16 (Unix-domain socket).
Usage: python3 examples/inspector_client.py /tmp/mash16.sock

Contains small helpers: send_request, get_registers, read_memory, subscribe
"""
import sys
import socket
import json
import base64
import time

SOCKET_PATH = sys.argv[1] if len(sys.argv) > 1 else "/tmp/mash16.sock"


def send_request(sock, method, params=None, timeout=2.0):
    req = {"method": method}
    if params:
        req.update(params)
    data = json.dumps(req).encode('utf-8')
    sock.sendall(data)
    # half-close to signal EOF for request-response methods
    try:
        sock.shutdown(socket.SHUT_WR)
    except Exception:
        pass
    # read response
    resp = b""
    sock.settimeout(timeout)
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            resp += chunk
    except socket.timeout:
        pass
    if not resp:
        return None
    try:
        return json.loads(resp.decode('utf-8'))
    except Exception:
        print("invalid json response:\n", resp)
        return None


def get_registers(path=SOCKET_PATH):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(path)
        return send_request(s, "getRegisters")


def read_memory(path, addr, size=64):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
        s.connect(path)
        return send_request(s, "readMemory", {"addr": addr, "size": size})


def subscribe(path=SOCKET_PATH, duration=5):
    # subscribe keeps the connection open and streams newline-delimited JSON events
    s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    s.connect(path)
    s.settimeout(1.0)
    s.sendall(b'{"method":"subscribe"}')
    s.shutdown(socket.SHUT_WR)
    start = time.time()
    buf = b""
    try:
        while time.time() - start < duration:
            try:
                chunk = s.recv(4096)
            except socket.timeout:
                continue
            if not chunk:
                break
            buf += chunk
            # process newline-delimited JSON lines
            while b'\n' in buf:
                line, buf = buf.split(b'\n', 1)
                if not line.strip():
                    continue
                try:
                    ev = json.loads(line.decode('utf-8'))
                    print("EVENT:", json.dumps(ev, indent=2))
                except Exception:
                    print("raw:", line)
    finally:
        s.close()


if __name__ == '__main__':
    print("get_registers:", get_registers())
    print("read_memory:", read_memory(SOCKET_PATH, 0, 16))
    print("subscribe (5s):")
    subscribe(SOCKET_PATH, duration=5)
