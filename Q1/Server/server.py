#Server version 2

from utils import sql_utils
import socket, threading
from os import path

import client_handler

DEFAULT_PORT = "1234"
PORT_FILE_PATH = "port.info"


def get_port():
	try:
		if not path.isfile((PORT_FILE_PATH)):
			with open(PORT_FILE_PATH, "x") as file:
				file.write(DEFAULT_PORT)
				return int(DEFAULT_PORT)

		with open(PORT_FILE_PATH, "r") as file:
			# If the port number is invalid then the binding will fail later.
			return int(file.read())

	# Let other exception raise higher
	except ValueError:
		print("Hello")
		# TODO: Handle invalid value in existing file


def run_server(port_number : int):
	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	server.bind(("0.0.0.0", port_number))

	threads = []

	while True:
		server.listen(1)
		connection, address = server.accept()
		new_thread = client_handler.ClientThread(connection)
		new_thread.start()
		threads.append(new_thread)


if __name__ == '__main__':
	port = get_port()
	sql_utils.init()
	run_server(port)
