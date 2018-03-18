#!/usr/bin/env python

import socket
import struct

KIWI_SERVER = ('127.0.0.1', 12312)


def main():
    client_hello_struct = struct.Struct('> I I I')
    client_hello = client_hello_struct.pack(1, 0xE6955EBF, 1)

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(KIWI_SERVER)
    sock.send(client_hello)
    sock.close()


if __name__ == '__main__':
    main()
