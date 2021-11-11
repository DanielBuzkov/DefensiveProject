#Server version 1

import socket
import os
import threading

from client_handler import ClientHandler

PORT_FILE_PATH = "port.info"
MAX_PORT_VALUE = 65535
DEFAULT_IP = "0.0.0.0"

'''
	The function will raise an exception upon failure which will not be
	caught by the caller since the server needs the port number to continue.
'''
def get_port():
	# if the file doesn't exist.
	if not os.path.isfile(PORT_FILE_PATH):
		raise ValueError("Info file can't be open (does file exists?)")

	with open(PORT_FILE_PATH, 'r') as portFile:
		data = portFile.read().split()

	# The file should contain one word.
	if len(data) != 1:
		raise ValueError("Invalid info file format")

	# Cast string to port and initialize the server.
	port = int(data[0])

	# Validate read value.
	if port < 1 or port > MAX_PORT_VALUE:
		raise ValueError("Invalid port value")

	return port


def run_server(port_number: int):
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.bind((DEFAULT_IP, port_number))
	sock.listen(50)

	client_handler = ClientHandler()

	while True:
		client_socket, addr = sock.accept()
		threading.Thread(target=client_handler.handle_client_thread, args=(client_socket,)).start()

	sock.close()


if __name__ == '__main__':
	port = get_port()
	run_server(port)
