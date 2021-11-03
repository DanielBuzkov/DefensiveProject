import sqlite3

DATABASE_NAME = "server.db"


class SqlHandler:
	def __init__(self):
		self.conn = sqlite3.connect(DATABASE_NAME)
		self.c = self.conn.cursor()
