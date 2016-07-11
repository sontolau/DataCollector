import json
import logging
import socket
import threading
from time import timezone, time
import time

import select

import signal

from task import *
from payload import *

class JsonPayload(Payload):
    """
    an implementation of Paylaod class, which is used to
    process JSON format data.
    """
    def payload(self, data, *args, **kwargs):
        if isinstance(data, str):
            raise TypeError()

        return json.loads(data)

    def process(self, payload):
        if isinstance(payload, dict):
            raise TypeError("")

        command = payload.get('command')
        cseq = payload.get('cseq')
        sessionkey = payload.get('sessionkey')
        args = payload.get('arguments')

        if not command:
            return "response", cseq, sessionkey, args
        else:
            if command in ("connect", "disconnect", "ping"):
                return command, cseq, sessionkey, args
            else:
                return "request", cseq, sessionkey, args

    def serialize(self, **kwargs):
        return json.dumps(kwargs)


class Listener(object):
    """
    the listener includes a set of functions called by SessionManager.
    """
    def onAuthenticate(self, peer, secret_key):
        """
        the function is used to authenticate a remote connection,
        if everything is ok, a session key will be returned.
        :param peer: see Peer for details.
        :param secret_key: a string used to authenticate.
        :return: a session key.
        """
        return None

    def onConnect(self, session):
        """
        this callback is called when a session is generated successfully.
        :param session: an instance of Session object.
        :return:
        """
        pass

    def onDisconnect(self, session):
        """
        called when an existed session will be closed.
        :param session:
        :return:
        """
        pass

    def onRequest(self, session, request):
        """
        called when an request arrived from remote with an existed session.
        :param session:
        :param request: Request object.
        :return:
        """
        pass

    def onResponseArrived(self, session, response):
        """
        called when an response arrived from remote with an existed session.
        :param session:
        :param response:
        :return:
        """
        pass

    def onResponseTimeout(self, session, request):
        """
        called when an request from server side is timed out.
        :param session:
        :param request:
        :return:
        """
        pass


class Peer(object):
    def __init__(self, sockfd, address=None):
        self.sock_fd = sockfd
        # self.sessions = []
        self.session = None
        self.last_update = 0
        self.address = address


class Session(object):
    def __init__(self, peer, session_key):
        self.session_key = session_key
        self.create_time = timezone.now
        self.cseq = 0
        self.peer = peer


class Request(object):
    def __init__(self, **kwargs):
        for k in kwargs.keys():
            setattr(self, k, kwargs[k])
        self.start_time = timezone.now


class Response(Request):
    def __init__(self, request, **kwargs):
        super(Response, self).__init__(**kwargs)
        self.request = request


class SessionManager(TaskManager):
    def sendPayload(self, peer, **kwargs):
        """
        send payload to remote connection.
        :param peer:
        :param kwargs:
        :return:
        """
        data = self.payload.serialize(**kwargs)
        self.write_task("OUT", Task(self._handle_out, args=(peer, data)))

    def acceptPeer(self, local_fd):
        """
        accept a new peer.
        :param local_fd:
        :return:
        """
        new_fd, (host, port) = local_fd.accept()
        logging.info("Connection established: <<<<<<< %s,%d" % (host, port))
        peer = self.newPeer(new_fd)
        if not peer:
            new_fd.close()
            return

        peer.address = (host, port)
        self._update_peer(peer)
        self.epoll.register(new_fd.fileno(), select.EPOLLIN | select.EPOLLPRI | select.EPOLLERR)
        return peer

    def _update_peer(self, peer):
        try:
            self.connections[1].remove(peer)
        except:
            pass

        self.connections[1].append(peer)
        peer.last_update = self.counter

    def newPeer(self, fd, peer=None):
        """
        return an existed peer or create new peer with specific file descriptor.
        :param fd:
        :param peer:
        :return:
        """
        if isinstance(fd, Peer):
            return fd
        elif isinstance(fd, int):
            try:
                return self.connections[0][fd]
            except:
                if self.max_clients>0 and len(self.connections[1]) > self.max_clients:
                    return None

                self.connections[0][fd] = peer
                return peer
        elif isinstance(fd, socket.socket):
            try:
                return self.connections[0][fd.fileno()]
            except:
                if self.max_clients > 0 and len(self.connections[1]) > self.max_clients:
                    return None

                peer = Peer(fd)
                self.connections[0][fd.fileno()] = peer
                return peer

    def closePeer(self, peer):
        """
        close remote connection.
        :param peer:
        :return:
        """
        logging.info("Closing connection from %s:%d." % (peer.address[0], peer.address[1]))
        peer = self.newPeer(peer)
        self.epoll.unregister(peer.sock_fd.fileno())
        # try:
        #     while len(peer.sessions) > 0:
        #         self.closeSession(peer.sessions[0])
        # except:
        #     pass
        if peer.session:
            self.closeSession(peer.session)
        try:
            del self.connections[0][peer.sock_fd.fileno()]
        except:
            pass
        try:
            self.connections[1].remove(peer)
        except:
            pass

        peer.sock_fd.close()
        del peer

    def processPeer(self, peer):
        """
        read from remote connection and process data.
        :param peer:
        :return:
        """
        peer = self.newPeer(peer)

        if not peer:
            return

        try:
            bufdata = peer.sock_fd.recv(self.max_bytes)
            if len(bufdata) <= 0:
                raise IOError("")

            self._update_peer(peer)
            logging.debug("C-S: %s" % (str(bufdata)))

            self.write_task('IN', Task(self._handle_in,
                                       args=(peer, self.payload.payload(bufdata))))
        except:
            raise IOError("")

    def sendRequest(self, session, command, arguments, timeout=0):
        """
        send request to an existed session.
        :param session:
        :param command:
        :param arguments:
        :param timeout:
        :return:
        """
        request = Request(command=command,
                                         cseq=self.cseq,
                                         sessionkey=session.session_key,
                                         arguments=arguments)
        setattr(request, "session", session)
        setattr(request, "start_time", self.counter)
        setattr(request, "timeout", timeout)
        self.sendPayload(session.peer,
                         command=command,
                         cseq=self.cseq,
                         sessionkey=session.session_key,
                         arguments=arguments)
        self.requests[self.cseq] = request
        self.cseq += 1
        return request

    def _handle_in(self, fd, payload):
        try:
            peer = self.newPeer(fd)
            event, cseq, sessionkey, args = self.payload.process(payload)
            if event == "connect":
                secretkey = args.get('scretkey')
                sessionkey = self.listener.onAuthenticate(peer, secretkey)
                if not secretkey:
                    self.sendPayload(peer, cseq=cseq, errcode="ERR_AUTHEN_FAILED")

                session = self.createSession(peer, sessionkey)
                if not session:
                    self.sendPayload(peer, cseq=cseq, errcode="ERR_SYSTEM_EXCEPTION")

                session.secretkey = secretkey
                self.sendPayload(peer, cseq=cseq, errcode="ERR_SUCCESS", sessionkey=sessionkey)
            elif event == "disconnect":
                session = self.getSession(sessionkey)
                if session:
                    self.closeSession(session)
            elif event == "ping":
                session = self.sessions.get(sessionkey)
                if session:
                    # self.listener.onPing(session)
                    self.sendPayload(peer,
                                     cseq=cseq,
                                     errcode="ERR_SUCCESS",
                                     sessionkey=sessionkey)
                else:
                    self.sendPayload(peer,
                                     cseq=cseq,
                                     errcode="ERR_SESSION_NOT_FOUND")
            elif event == "request":
                session = self.sessions.get(sessionkey)
                if not session:
                    self.sendPayload(peer, cseq=cseq, errcode="ERR_SESSION_NOT_FOUND")
                else:
                    self.listener.onRequest(session, Request(**args))
            elif event == "response":
                self.lock.acquire()
                request = self.requests.get(cseq)
                session = self.getSession(sessionkey)

                if request and session:
                    response = Response(request, **args)
                    if self.listener:
                        self.listener.onResponseArrived(session, response)
                try:
                    del request
                except:
                    pass
                pass


        except Exception as e:
            logging.error(e.message)
            self.closePeer(fd)
            return

    def _handle_out(self, peer, data):
        try:
            peer.sock_fd.send(data)
        except:
            pass
        pass

    def __init__(self,
                 address,
                 listener=Listener,
                 payload=JsonPayload,
                 in_queue=(100, 1),
                 out_queue = (100,1),
                 # max_sessions=0,
                 max_clients=0,
                 wait_timeout=0,
                 max_bytes=0xFFFF):
        super(SessionManager, self).__init__(TaskQueue("IN", in_queue[0], in_queue[1]),
                                             TaskQueue("OUT", out_queue[0], out_queue[1]))
        self.address = address
        self._exit_flag = False
        self.sock_fd = None
        self.lock = threading.Lock()
        self.requests = {}
        self.cseq = 0
        self.counter = 0
        self.sessions = {}
        self.connections = [{}, []]
        self.session_list = []
        # self.max_sessions = max_sessions
        self.max_clients = max_clients
        self.wait_timeout = wait_timeout
        self.max_bytes = max_bytes

        if listener:
            self.listener = listener()
            self.listener.manager = self

        self.payload = payload()
        self.payload.manager = self

    # create new session.
    def createSession(self, peer, sessionkey):
        """
        create a new session.
        :param peer:
        :param sessionkey:
        :return:
        """
        peer = self.newPeer(peer)

        # generate new session key.
        # skey = rand_str(SessionManager.KEY_LEN,
        #                 SessionManager.KEY_CHARACTERS)

        # allocate new session.
        session = Session(peer, sessionkey)
        self.sessions[sessionkey] = session
        if peer.session:
            logging.info("The session with key %s has existed already, now close and create a new one."%(peer.session.session_key))
            self.closeSession(peer.session)

        peer.session = session
        # peer.sessions.append(session)
        # session.secret_key = sessionkey
        if self.listener:
            try:
                self.listener.onConnect(session)
            except Exception as e:
                logging.error("Closing sessinon due to %s."%(e.message))
                self.closeSession(session)
                return None
        logging.info("Created new session %s successfully."%(session.session_key))
        return session

    def getSession(self, session_key):
        """
        return an existed session.
        if no session found, None will be returned.
        :param session_key:
        :return:
        """
        return self.sessions.get(session_key)

    def closeSession(self, session):
        """
        close an existed session.
        :param session:
        :return:
        """
        if self.listener:
            self.listener.onDisconnect(session)

        if self.sessions.has_key(session.session_key):
            del self.sessions[session.session_key]

        try:
            session.peer.session=None
            # session.peer.sessions.remove(session)
        except:
            pass
        logging.info("Closed session %s."%(session.session_key))
        del session

    def _watch_dog(self):
        # import time
        while not self._exit_flag:
            time.sleep(1)
            self.counter += 1

            self.lock.acquire()

            if self.wait_timeout > 0 and self.counter % self.wait_timeout == 0:
                logging.debug("Checking connection...")
                for peer in self.connections[1]:
                    if self.counter - peer.last_update > self.wait_timeout:
                        logging.info("Closing remote connection due to read timedout.")
                        self.closePeer(peer)
                    else:
                        break

            for r in self.requests.keys():
                request = self.requests[r]
                if request.timeout > 0 and self.counter - request.start_time > request.timeout:
                    logging.info ("Requested (%s:%s) timed out."%(request.command, request.session_key))
                    if self.listener:
                        self.listener.onResponseTimeout(request.session, request)
                    del self.requests[r]
                    del request

            self.lock.release()

    # def _signal_proc(self, sig):
    #     if sig in (signal.SIGINT, signal.SIGTERM):
    #         self._exit_flag = True

    def start(self):
        """
        start session manager.
        :return:
        """
        super(SessionManager, self).start()
        # create TCP socket server.
        self.epoll = select.epoll()
        self.sock_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_IP)
        self.sock_fd.setblocking(False)
        self.sock_fd.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, True)
        self.sock_fd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
        self.sock_fd.bind(self.address)
        self.sock_fd.listen(500)
        self.epoll.register(self.sock_fd.fileno(), select.EPOLLIN | select.EPOLLPRI | select.EPOLLERR)

        # start watch dog thread to inspect system.
        self.watch_dog = threading.Thread(target=self._watch_dog, name="watch_dog")
        self.watch_dog.setDaemon(True)
        self.watch_dog.start()

        # set signal processor.


        # loop core.
        while not self._exit_flag:
            try:
                ev_fds = self.epoll.poll(1.0, maxevents=-1)
            except IOError:
                continue

            if len(ev_fds) <= 0:  # if timedout
                continue
            else:
                self.lock.acquire()
                for fd, ev in ev_fds:
                    try:
                        if ev & select.EPOLLIN:
                            if fd == self.sock_fd.fileno():
                                self.acceptPeer(self.sock_fd)
                            else:
                                data = self.processPeer(fd)
                                if data:
                                    self.write_task('IN', Task(self._handle_in, args=(fd, data)))
                        elif ev & select.EPOLLERR or ev & select.EPOLLHUP:
                            raise IOError("")
                        else:
                            pass
                    except IOError as e:
                        logging.error(e.message)
                        self.closePeer(self.newPeer(fd))
                self.lock.release()

        self.watch_dog.join(2)
        self.sock_fd.close()
        self.epoll.close()
        super(SessionManager, self).stop()

    def stop(self, *args):
        self._exit_flag = True


if __name__ == "__main__":
    import sys
    def sig_proc(sig, f):
        sessionManager.stop()
    logging.basicConfig(level=logging.DEBUG)

    sessionManager = SessionManager(address=(sys.argv[1], int(sys.argv[2])))
    signal.signal(signal.SIGTERM, sig_proc)
    signal.signal(signal.SIGINT, sig_proc)
    signal.signal(signal.SIGPIPE, signal.SIG_IGN)
    signal.signal(signal.SIGHUP, signal.SIG_IGN)
    sessionManager.start()

    pass
