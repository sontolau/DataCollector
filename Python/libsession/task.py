import Queue
import logging
import threading

__all__ = ['Task', 'TaskQueue', 'TaskManager']


class Task(object):
    def __init__(self, target, args=None):
        self.target = target
        self.args = args


class TaskQueue(object):
    def __init__(self, tag, size, threads):
        self.tag = tag
        self.size = size
        self._queue = Queue.Queue(size)
        self.threads = threads
        # self._locker = threading.Lock()
        self._threads = []
        self._exit_flag = False

    def _proc(self, *args):
        logging.debug("[Tag] %s: started already." % (self.tag))
        while True:
            try:
                if self._exit_flag:
                    break
                task = self._queue.get(True, 1)
                logging.debug("[Tag] %s: processing task ..." % (self.tag))
                task.target(task.args)
                del task
            except Queue.Empty as e:
                logging.debug("Checking.")
                continue
            finally:
                pass

        logging.debug("[Tag] %s: exited." % (self.tag))

    def write_task(self, task, block=True, timeout=None):
        if not isinstance(task, Task):
            raise TypeError(Task.__name__)

        try:
            if self._exit_flag:
                raise Exception
            self._queue.put(task, block, timeout)
        except Exception as e:
            logging.error(e.message)
        finally:
            pass

    def start(self):
        for i in range(0, self.threads):
            th = threading.Thread(target=self._proc,
                                  name="%s %d" % (self.tag, i))
            th.setDaemon(True)
            th.start()
            self._threads.append(th)

    def stop(self):
        self._exit_flag = True

        try:
            while True:
                th = self._threads.pop()
                if th.is_alive:
                    th.join(1)
                del th
        except:
            pass


# logging.basicConfig(level=logging.DEBUG, stream=sys.stdout)
#
# taskQueue = TaskQueue("In", 100, 5)
# taskQueue.start()
#
# def process_task(argument,):
#     logging.debug(argument)
#     pass
#
# def write_task(*args):
#     while True:
#         task = Task(target=process_task, args=("hello"))
#         taskQueue.writeTask(task)
#     pass
#
# threading.Thread(target=write_task, args=()).start()
#
# time.sleep(5)
# logging.debug("Stop")
# taskQueue.stop()



class TaskManager(object):
    def __init__(self, *tasks):
        self._tasks = {}
        for task in tasks:
            self._tasks[task.tag] = task

    def start(self):
        for tag in self._tasks.keys():
            task = self._tasks[tag]
            task.start()

    def remove(self, tag=None):
        if tag:
            self._tasks[tag].stop()
        else:
            for k in self._tasks.keys():
                self._tasks[k].stop()
                del self._tasks[k]


    def stop(self, tag=None):
        if tag:
            self._tasks[tag].stop()
        else:
            for k in self._tasks.keys():
                self._tasks[k].stop()

    def write_task(self, tag, task):
        self._tasks[tag].write_task(task)