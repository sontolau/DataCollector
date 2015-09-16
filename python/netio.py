#!/usr/bin/env python

import thread,socket,select,sys
import Queue
import threading

class netio:
    PROTO_TCP = 1
    PROTO_UDP = 2

    class net_buffer:
        def __init__(self, strdata, tag=None, addrinfo=None, io=None):
            self.data = strdata
            self.tag  = tag
            self.addrinfo = addrinfo
            self.io = io

    class delegate:
        def did_connect_host(self, conn):
            pass
        def did_wait_timedout(self, conn):
            pass
        def did_recv_data(self, conn, data):
            pass
        def did_send_data(self, conn, data):
            pass
        def did_disconnect_host(self, conn):
            pass
        def did_accept_remote(self, conn, sock):
            pass

    def __init__(self, host, port, bind=False, maxbufsz=6556, delegate=delegate(), timedout=3):
        self.host = host
        self.port = port
        self.buffer_size = maxbufsz
        self.delegate = delegate
        self.is_connected = False
        self.timedout = timedout
        self.__buffer_queue = Queue.Queue()
        self.__proc_buf_queue = Queue.Queue()
        self.__notifier = threading.Condition()
        self.__proc_notifier = threading.Condition()

        self.__select_fds = []
        self.__bind     = bind

    def __read_thread(self, *args):
        while True:
            readylist = select.select(self.__select_fds, [], [], self.timedout)
            if len(readylist[0]) == 0:
                self.delegate.did_wait_timedout(self)
                continue
            for sock_fd in readylist[0]:
                try:
                    if self.__bind \
                        and self.__proto == netio.PROTO_TCP \
                        and sock_fd == self.__sockfd:
                            self.__accept_io(self.__sockfd)
                            continue
                    elif self.__proto == netio.PROTO_TCP:
                        data = sock_fd.recv(self.buffer_size)
                        if data is None or len(data) == 0:
                            self.__close_io(sock_fd)
                            continue
                        addrinfo = sock_fd.getpeername()
                    else:
                        (data, addrinfo) = sock_fd.recvfrom(self.buffer_size)

                    buf = netio.net_buffer(data, addrinfo=addrinfo, io=sock_fd)
                    self.__proc_buf_queue.put(buf, True)
                    self.__proc_notifier.acquire()
                    self.__proc_notifier.notifyAll()
                    self.__proc_notifier.release()
                    #self.delegate.did_recv_data(self, buf)
                except Exception as e:
                    print "an error occured due to %s"%str(e)
                    self.__close_io(sock_fd)

    def __write_thread(self, *args):
        while True:
            try:
                self.__notifier.acquire()
                self.__notifier.wait()
                self.__notifier.release()
                while self.__buffer_queue.not_empty:
                    buf = self.__buffer_queue.get(True)
                    if buf.io is None:
                        buf.io = self.__sockfd
                    try:
                        if self.__proto == netio.PROTO_TCP:
                            szsend = buf.io.send(buf.data)
                        else:
                            szsend = buf.io.sendto(buf.data, (self.host, self.port))
                        if szsend < 0:
                            self.__close_io(buf.io)
                        else:
                            self.delegate.did_send_data(self, buf)
                    except Exception as e:
                        print str(e)
                        self.__close_io(buf.io)
            except Exception as e:
                print str(e)
                pass

    def __proc_thread(self, *args):
        while True:
            try:
                self.__proc_notifier.acquire()
                self.__proc_notifier.wait()
                self.__proc_notifier.release()
                while self.__proc_buf_queue.not_empty:
                    buf = self.__proc_buf_queue.get(True)
                    self.delegate.did_recv_data(self, buf)
            except Exception as e:
                print str(e)

    def __conn_async(self, *args):
        try:
            if self.__bind:
                if self.__proto == netio.PROTO_TCP:
                    self.__sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    self.__sockfd.bind((self.host, self.port))
                    self.__sockfd.listen(100)
                else:
                    self.__sockfd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                    self.__sockfd.bind((self.host, self.port))
            else:
                if self.__proto == netio.PROTO_TCP:
                    self.__sockfd = socket.create_connection((self.host, self.port))
                else:
                    self.__sockfd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            assert self.__sockfd is not None

            self.__select_fds.append(self.__sockfd)
            self.__sockfd.setblocking(False)
            thread.start_new_thread(self.__write_thread, (self,))
            thread.start_new_thread(self.__read_thread, (self,))
            thread.start_new_thread(self.__proc_thread, (self,))
            self.delegate.did_connect_host(self)

        except Exception as e:
            print str(e)
            sys.exit(1)



    def start(self, proto=PROTO_TCP):
        try:
            self.__proto = proto
            thread.start_new_thread(self.__conn_async, (self,))
            return True
        except Exception as e:
            print "start new thread failed due to %s" % str(e)
            return False

    def __accept_io(self, sock):
        self.__notifier.acquire()
        try:
            remote = sock.accept()
            self.__select_fds.append(remote)
            self.delegate.did_accept_remote(self, remote)
        except Exception as e:
            print str(e)
            return False
        finally:
            self.__notifier.release()
        return True

    def __close_io(self, sock):
        try:
            self.__select_fds.remove(sock)
            sock.close()
            if self.is_connected:
                self.delegate.did_disconnect_host(self)
            print "closed connection successfully"
        except Exception as e:
            print str(e)


    def write(self, buf):
        try:
            self.__buffer_queue.put(buf, True)
            self.__notifier.acquire()
            self.__notifier.notifyAll()
            self.__notifier.release()
            return True

        except Exception as e:
            print str(e)
            return False


