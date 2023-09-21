from sync_server import address, authkey
from multiprocessing.managers import BaseManager

BaseManager.register("get_queue")
BaseManager.register("get_event")

manager = BaseManager(authkey=authkey, address=address)
manager.connect()

queue= manager.get_queue()
event = manager.get_event()

print(queue.get())  