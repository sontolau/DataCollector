import threading
import time

class Scheduler(threading.Thread):
    class _TimerDesc:
        pass

    def __init__(self):
        super(Scheduler, self).__init__()
        self._timers = {}
        self.locker = threading.Lock()
        self.setDaemon(True)
        self._exit_flag = False

    def run(self):
        class _SubCall(threading.Thread):
            def __init__(self, task):
                super(_SubCall, self).__init__()
                self.task = task
                self.setDaemon(True)
                self.start()

            def run(self):
                self.task.target(*self.task.args)

        # timer = 0
        while True:
            # timer += 1
            self.locker.acquire()

            try:
                if self._exit_flag:
                    break

                for k in self._timers.keys():
                    cur_time = int(time.time())
                    tm = self._timers[k]

                    if cur_time >= tm.next_sched_seconds:
                        if not tm.block or (tm.task is None and tm.block):
                            tm.next_sched_seconds = cur_time + tm.timer
                            tm.task = _SubCall(tm)
                    #
                    #     if hasattr(tm, '_running_proc') and tm._running_proc.isAlive():
                    #         continue
                    #
                    # if (timer % int(k) == 0):
                    #     for sub in self._sched_list[k].keys():
                    #         task = self._sched_list[k][sub]
                    #         if hasattr(task, '_running_proc') and task._running_proc.isAlive():
                    #             continue
                    #
                    #         task._running_proc = _SubCall(task)

            finally:
                self.locker.release()
            time.sleep(1)

    def register(self, tag, timer, target, block=True, args=None):
        if timer <= 0:
            raise ValueError('')

        with self.locker:
            # if self._sched_list.get(timer) is None:
            #     self._sched_list[timer] = {}
            _tm = Scheduler._TimerDesc()
            _tm.target = target
            _tm.args = args
            _tm.block = block
            _tm.next_sched_seconds = int(time.time()) + timer
            _tm.task = None
            _tm.timer = timer


            if self._timers.has_key(tag):
                self._timers[tag] = _tm

            self._timers[tag] = _tm
            #
            # if self._sched_list[timer].has_key(tag):
            #     del self._sched_list[timer][tag]
            #
            # self._sched_list[timer][tag] = _tm
        pass

    def deregister(self, tag):
        with self.locker:
            try:
                del self._timers[tag]

            except:
                pass
    def stop(self):
        with self.locker:
            self._exit_flag = True

#
# def default_scheduler():
#     global _scheduler
#     if _scheduler is None:
#         _scheduler = Scheduler()
#
#     return _scheduler
#
# if __name__ == "__main__":
#     def print_xxxx(**kwargs):
#         print (str(kwargs))
#
#     s = Scheduler()
#     s.register(1, "print 1", print_xxxx, args=(1,2))
#     s.register(1, "print 1", print_xxxx)
#
#     s.register(2, "print 2", print_xxxx)
#     time.sleep(100)
