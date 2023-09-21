from multiprocessing.managers import BaseManager
from multiprocessing import Queue
from multiprocessing import Event

address = ('127.0.0.1', 50002)  # you can change this
authkey = b"PeterYaoNYUShanghai"  # you should change this


class SharedSync:

    def __init__(self):
        self._queue = Queue()
        self._event = Event()

    def __call__(self):
        return self._queue
    
    def get_queue(self):
        return self._queue
    
    def get_event(self):
        return self._event


if __name__ == "__main__":
    # Register our queue
    shared_sync = SharedSync()
    BaseManager.register("get_queue", shared_sync.get_queue)
    BaseManager.register("get_event", shared_sync.get_event)

    # Start server
    manager = BaseManager(address=address, authkey=authkey)
    srv = manager.get_server()
    srv.serve_forever()