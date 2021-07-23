#!/usr/bin/python3

"""
Run python test cases on the e32 for the control socket.
For these to run the e32 needs to be running with the following
socket files below.

Run the e32 like so:
$ e32 -v --sock-unix-ctrl $HOME/e32.control --sock-unix-data $HOME/e32.data

Then run the test like so:
$ python3 ebyte-sx1276/test/e32_test.py 
"""

import socket
import os
import os.path
import unittest
from pathlib import Path

CONTROL_SOCKET_FILE = str(Path.home())+"/e32.control"
DATA_SOCKET_FILE = str(Path.home())+"/e32.data"
CLIENT_SOCKET_FILE = str(Path.home())+"/client"

def open_client_socket():
    """ create a new socket file if already there """

    client_socket = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    client_socket.bind(CLIENT_SOCKET_FILE)

    return client_socket

def close_client_socket(client_socket, client_socket_file):
    """ close the socket and remove the file """
    client_socket.close()
    if os.path.exists(client_socket_file):
        os.remove(client_socket_file)

class TestE32ControlSocket(unittest.TestCase):
    """ A class to test the control socket of the e32 """

    def setUp(self):
        """ Open the client socket each time """
        self.client_socket = open_client_socket()
    
    def tearDown(self):
        """ close the socket """
        close_client_socket(self.client_socket, CLIENT_SOCKET_FILE)

    def test_aaa_server_socket(self):
        """ Test the socket exists """
        self.assertTrue(os.path.exists(CONTROL_SOCKET_FILE))

    def test_get_version(self):
        """ get the version """
        self.client_socket.sendto(b'v', CONTROL_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(6)
        #self.assertEquals(address, CONTROL_SOCKET_FILE)
        self.assertEqual(len(bytes), 4)

    def test_get_settings(self):
        """ get the settings """
        self.client_socket.sendto(b's', CONTROL_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(6)
        #self.assertEquals(address, CONTROL_SOCKET_FILE)
        self.assertEqual(len(bytes), 6)
        self.assertEqual(bytes[0] | 0x03, 0xc3)

    def test_get_invalid(self):
        """ test an invalid command through the socket """
        self.client_socket.sendto(b'i', CONTROL_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(6)
        #self.assertEquals(address, CONTROL_SOCKET_FILE)
        self.assertEqual(len(bytes), 1)
        self.assertEqual(bytes[0], 7)

    def test_send_too_many_bytes(self):
        """ test sending another form of an invalid command """
        self.client_socket.sendto(b'12345678901234567890', CONTROL_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(6)
        #self.assertEquals(address, CONTROL_SOCKET_FILE)
        self.assertEqual(len(bytes), 1)
        self.assertEqual(bytes[0], 7)

    def test_change_settings(self):
        """ get the settings, change them, change them back to original """

        self.client_socket.sendto(b's', CONTROL_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(6)
        #self.assertEquals(address, CONTROL_SOCKET_FILE)
        self.assertEqual(len(bytes), 6)
        self.assertEqual(bytes[0] | 0x03, 0xc3)

        bytes_orig = bytes
        bytes_new = bytearray(bytes)  # make it mutable

        # change the settings
        address_high = 0x01
        address_low = 0x0f
        bytes_new[1] = address_high
        bytes_new[2] = address_low

        # change the settings
        self.client_socket.sendto(bytes_new, CONTROL_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(6)
        #self.assertEquals(address, CONTROL_SOCKET_FILE)
        self.assertEqual(len(bytes), 6)
        self.assertEqual(bytes[0] | 0x03, 0xc3)
        self.assertEqual(bytes[1], address_high)
        self.assertEqual(bytes[2], address_low)

        # change it back to the original
        self.client_socket.sendto(bytes_orig, CONTROL_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(6)
        #self.assertEquals(address, CONTROL_SOCKET_FILE)
        self.assertEqual(len(bytes), 6)
        self.assertEqual(bytes[0] | 0x03, 0xc3)
        self.assertEqual(bytes[1], bytes_orig[1])
        self.assertEqual(bytes[2], bytes_orig[2])

        # TODO we will run tearDown and remove the file before the e32 can send back
        # from the e32 we get a
        # ERROR [ENOENT No such file or directory] unable to send back status to unix socket
        # time.sleep(1)

class TestE32DataSocket(unittest.TestCase):
    """ A class to test the data socket of the e32 """

    def setUp(self):
        """ Open the client socket each time """
        self.client_socket = open_client_socket()

    def tearDown(self):
        """ close the socket """
        close_client_socket(self.client_socket, CLIENT_SOCKET_FILE)

    def test_good_data(self):
        """ send good data to the data socket """
        self.client_socket.sendto(b'', DATA_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(10)

        #self.assertEquals(address, DATA_SOCKET_FILE)

        # assert a successful registration
        self.assertEqual(len(bytes), 1)
        self.assertEqual(bytes[0], 0)

        data = b'hello world!'
        self.client_socket.sendto(data, DATA_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(10)
        # assert a successful transmission
        self.assertEqual(len(bytes), 1)
        self.assertEqual(bytes[0], 0)

    def test_data_too_big(self):
        """ send too much to the data socket """
        self.client_socket.sendto(b'', DATA_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(10)

        #self.assertEquals(address, DATA_SOCKET_FILE)

        # assert a successful registration
        self.assertEqual(len(bytes), 1)
        self.assertEqual(bytes[0], 0)

        data = bytearray(100)
        self.assertEqual(len(data), 100)
        self.client_socket.sendto(data, DATA_SOCKET_FILE)
        (bytes, address) = self.client_socket.recvfrom(10)
        # assert a successful transmission
        self.assertEqual(len(bytes), 1)
        self.assertEqual(bytes[0], 1)

# settings is c0 00 01 1a 17 44
# version is c3 45 0d 14
if __name__ == '__main__':
    unittest.main()
