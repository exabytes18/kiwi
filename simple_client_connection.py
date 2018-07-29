#!/usr/bin/env python

from contextlib import closing
import socket
import struct

KIWI_SERVER = ('127.0.0.1', 12312)


def client_hello():
    client_hello_struct = struct.Struct('> I I I')
    client_hello = client_hello_struct.pack(0x40000000, 0xE6955EBF, 1)

    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        sock.connect(KIWI_SERVER)
        sock.send(client_hello)

        response_data = sock.recv(64*1024)
        message_type, = struct.unpack('> I', response_data)
        assert message_type == 0x40000001, "Expected message_type = %s, got %s instead" % (0x40000001, message_type)


def client_hello_bad_magic_number():
    client_hello_struct = struct.Struct('> I I I')
    client_hello = client_hello_struct.pack(0x40000000, 0xBADDBADD, 1)

    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(KIWI_SERVER)
        sock.send(client_hello)

        response_data = sock.recv(64*1024)
        message_type, error_code, error_message_length = struct.unpack_from('> I I H', response_data)
        assert message_type == 0x00000001, "Expected message_type = %s, got %s instead" % (0x00000001, message_type)
        assert error_code == 1, "Expected error_code = %s, got %s instead" % (1, error_code)


if __name__ == '__main__':
    client_hello()
    client_hello_bad_magic_number()
