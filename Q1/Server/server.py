#Server version 2

import socket
import os

from utils.sql_handler import SqlHandler
from client_handler import ClientHandler

PORT_FILE_PATH = "port.info"
MAX_PORT_VALUE = 65535

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
	sock.bind(("0.0.0.0", port_number))
	sock.listen(50)

	handler = ClientHandler()
	# sql_handler = MutexableSql()

	while True:
		client_socket, addr = sock.accept()

		handler.handle_client(client_socket)

		client_socket.close()

		# new_thread = client_handler.ClientThread(connection)
		# new_thread.start()
		# threads.append(new_thread)

	sock.close()


if __name__ == '__main__':
	port = get_port()
	run_server(port)
