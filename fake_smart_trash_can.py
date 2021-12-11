#!/usr/bin/env python3

# for debugging purpose

import argparse
import io
import socket
import struct

parser = argparse.ArgumentParser()
parser.add_argument('device_id', type=int)
parser.add_argument('weight', type=int)
parser.add_argument('utilization', type=int)
parser.add_argument('image', type=str)
args = parser.parse_args()

with io.BytesIO() as fb, open(args.image, "rb") as fin:
    fb.write(struct.pack("<LHH", args.device_id, args.weight, args.utilization))
    fb.write(fin.read())

    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("127.0.0.1", 5000))

    s.send(b"POST /update HTTP/1.1\r\n")
    s.send(b"Host: 127.0.0.1\r\n")
    s.send(b"Connection: close\r\n")
    s.send(b"Content-Type: application/octet-stream\r\n")
    s.send(b"Content-Length: %d\r\n" % fb.tell())
    s.send(b"\r\n")
    s.send(fb.getbuffer())

    while len(s.recv(1024)) > 0:
        continue

    s.close()
