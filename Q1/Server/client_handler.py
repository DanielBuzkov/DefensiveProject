from threading import Thread


class ClientThread(Thread):
    def __init__(self, conn, address):
        Thread.__init__(self)
        self.address = address
        self.conn = conn

    def run(self):
        while True:
            data = self.conn.recv(2048)