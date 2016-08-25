import Queue
import logging
import threading

from libsession import Log


class Task(object):
    def __init__(self, target, *args):
        """
        :param target: a function that will be called to process self.
        :param args: a list of arguments.
        """
        self.target = target
        self.args = args


class TaskQueue(object):
    def __init__(self, tag, size, threads):
        """
        :param tag: specify a tag to process.
        :param size: the size of the queue.
        :param threads: how many threads will be created to process this queue.
        """
        self.tag = tag
        self.size = size
        self._queue = Queue.Queue(size)
        self.threads = threads
        # self._locker = threading.Lock()
        self._threads = []
        self._exit_flag = False

    def _proc(self, *args):
        # logging.debug("[Tag] %s: started already." % (self.tag))
        Log.d(__package__,
              object=self.__class__.__name__,
              log="Task %s started already successfully." % (self.tag))
        while True:
            try:
                if self._exit_flag:
                    break
                task = self._queue.get(True, 1)
                task.target(*task.args)
                del task
            except Queue.Empty as e:
                # logging.debug("Checking.")
                continue
            except Exception as e:
                Log.e(__package__, object=self.__class__.__name__, log=e.message)
            finally:
                pass
        Log.d(__package__,
              object=self.__class__.__name__,
              log="Task %s stopped." % (self.tag))

    def write_task(self, task, block=True, timeout=None):
        """
        append a task into current queue to process.
        :param task: an instance of Task class.
        :param block:
        :param timeout:
        :return:
        """
        if not isinstance(task, Task):
            raise TypeError(Task.__name__)

        try:
            if self._exit_flag:
                raise Exception
            self._queue.put(task, block, timeout)
        except Exception as e:
            Log.e(__package__, object=self.__class__.__name__, log=e.message)
        finally:
            pass

    def start(self):
        """
        start to process queue.
        :return:
        """
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


class TaskManager(object):
    def __init__(self, *tasks):
        """
        :param tasks: task1, task2, ...,task n
        task is kind of TaskQueue instance.
        """
        self._tasks = {}
        for task in tasks:
            if not isinstance(task, TaskQueue):
                raise TypeError("not instance of TaskQueue")
            self._tasks[task.tag] = task

    def start(self):
        """
        start task manager.
        :return:
        """
        for tag in self._tasks.keys():
            task = self._tasks[tag]
            task.start()

    def remove(self, tag=None):
        """
        remove an existed task queue related to the tag.
        if tak is None, then remove all.
        :param tag: the tag of task queue.
        :return:
        """
        if tag:
            self._tasks[tag].stop()
        else:
            for k in self._tasks.keys():
                self._tasks[k].stop()
                del self._tasks[k]

    def stop(self, tag=None):
        """
        stop an active task queue related to the tag,
        if tag is None, stop all.
        :param tag:
        :return:
        """
        if tag:
            self._tasks[tag].stop()
        else:
            for k in self._tasks.keys():
                self._tasks[k].stop()

    def write_task(self, tag, task):
        """
        add a task to the specific task queue.
        :param tag:
        :param task:
        :return:
        """
        if not isinstance(task, Task):
            raise TypeError("not instance of Task")
        self._tasks[tag].write_task(task)
