import json
import logging
import socket
import traceback
from time import time
import time
import select
import signal

from protocol import TCP, UDP, ProtoUnsupportedException
from schedule import Scheduler
from task import *
from payload import *


class JsonPayload(Payload):
    """
    an implementation of Paylaod class, which is used to
    process JSON format data.
    """

    def payload(self, data, *args, **kwargs):
        if not isinstance(data, str):
            raise TypeError("a json-like string is requred.")

        return json.loads(data)

    def process(self, payload):
        if not isinstance(payload, dict):
            raise TypeError("the payload must be dict type.")

        command = payload.get('command', None)
        cseq = payload.get('cseq')
        sessionkey = payload.get('sessionkey')
        args = payload.get('arguments', {})
        errcode = payload.get('errcode')
        return command, cseq, sessionkey, errcode, args

    def serialize(self, **kwargs):
        return json.dumps(kwargs)


class SessionUndoneException(Exception):
    pass


class Peer(object):
    DISCONNECTED = 0
    CONNECTING = 1
    CONNECTED = 2

    def __init__(self, sockfd, address=None, func=None):
        self.sock_fd = sockfd
        self.sessions = []
        self.ctime = time.time()
        self.last_read_time = self.ctime
        self.last_write_time = self.ctime
        self.last_connect_time = self.ctime
        self.address = address
        self.lock = threading.Lock()
        self.refcount = 1
        self.status = Peer.DISCONNECTED
        self._cb = func

    def get(self):
        with self.lock:
            self.refcount += 1

        return self

    def close(self):
        with self.lock:
            self.refcount -= 1

            if self.refcount <= 0:
                self.sock_fd.close()
                if self._cb is not None:
                    self._cb(self)

    def addSession(self, session):
        with self.lock:
            self.sessions.append(session)
            setattr(session, "peer", self)

    def removeSession(self, session):
        with self.lock:
            try:
                for s in self.sessions:
                    if s == session:
                        self.sessions.remove(s)
            except:
                pass

    def read(self, proto):
        with self.lock:
            data = proto.receive()
            self.last_read_time = time.time()
            return data

    def write(self, proto, data):
        with self.lock:
            proto.send(data)
            self.last_write_time = time.time()


class Session(object):
    DISCONNECTED = 0
    CONNECTING = 1
    CONNECTED = 2
    pass


class SessionProcessor(object):
    @classmethod
    def parseData(cls, data):
        pass

    @classmethod
    def isRequest(cls, data):
        pass

    @classmethod
    def isResponse(cls, data):
        pass

    @classmethod
    def getRequestType(cls, data):
        pass

    @classmethod
    def getSessionKey(cls, data):
        pass

    @classmethod
    def createSession(cls, peer, data):
        pass

        # @classmethod
        # def


class AuthenFailedException(Exception):
    pass


class DefaultSessionProcessor(SessionProcessor):
    @classmethod
    def parseData(cls, data):
        return json.loads(data)

    @classmethod
    def isRequest(cls, js):
        if js.get("command") != None:
            request = Request()
            request.command = js.get("command")
            request.cseq = js.get("cseq")
            request.session_key = js.get("session_key")
            request.arguments = js.get("arguments", {})
            return request, request.cseq

        return None, None

    @classmethod
    def isResponse(cls, js):
        if js.get("command") == None:
            resp = Response()
            resp.cseq = js.get("cseq")
            resp.session_key = js.get("session_key")
            resp.arguments = js.get("arguments", {})
            return resp, resp.cseq

        return None, None

    @classmethod
    def isConnected(cls, request):
        if request.command == "connect":
            return True, request.arguments.get("secret_key")
        return False, None

    @classmethod
    def isDisconnected(cls, request):
        if request.command == "disconnect":
            return True, request.arguments.get("session_key")
        return False, None

    @classmethod
    def isTransaction(cls, request):
        if request.command not in ("connect", "disconnect", "ping"):
            return True, request.session_key, request.arguments
        return False, None, None

    @classmethod
    def makeRequest(cls, **data):
        pass

    @classmethod
    def makeResponse(cls, **data):
        pass




    @classmethod
    def makeConnectRequest(cls, secretkey, **kwargs):
        pass

    @classmethod
    def makeDisconnectRequest(cls, ):
        pass

    @classmethod
    def disconnect(cls, session):
        pass

    @classmethod
    def request(cls, session, **data):
        pass

    @classmethod
    def response(cls, session, request, **data):
        pass

    # @classmethod
    # def createSession(cls, peer, js):
    #     secret_key = js.get("arguments", {}).get("secretkey", None)
    #     if cls.manager.listener.onAuthenticate(peer, secret_key):
    #         session = Session()
    #         session.peer = peer
    #         session.secret_key = secret_key
    #         session.status = Session.CONNECTED
    #         session.session_key = cls.manager.listener.onConnect(session)
    #         return session
    #     else:
    #         raise AuthenFailedException()

    @classmethod
    def writeSession(cls, session, **data):
        pass


class Request(object):
    pass

    #
    # for k in kwargs.keys():
    #     setattr(self, k, kwargs[k])
    # self.start_time = time.time()


class Response(object):
    def __init__(self, request, errcode, arguments):
        self.request = request
        self.errcode = errcode
        self.arguments = arguments

        # super(Response, self).__init__(request=request,
        #                                errcode=errcode,
        #                                arguments=kwargs)


class SessionManager(TaskManager):
    CONNECTING = 1
    CONNECTED = 2
    DISCONNECTED = 3

    class Config(object):
        bind = False
        protocol = TCP
        in_queue = (100, 1),
        out_queue = (100, 1),
        max_clients = 0,
        wait_timeout = (0, 0),
        max_bytes = 0xFFFF
        snd_buf_size = 0xFFFF
        rcv_buf_size = 0xFFFF
        snd_rcv_timeout = 0

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
            return True

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

        def onPing(self, session, **kwargs):
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

    def getConfValue(self, key, default):
        if self.options is not None:
            return self.options.get(key, default)

        return default

    def sendPayload(self, peer, **kwargs):
        """
        send payload to remote connection.
        :param peer:
        :param kwargs:
        :return:
        """
        data = self.payload.serialize(**kwargs)
        with self.lock:
            try:
                self.write_task("OUT", Task(self._handle_out, peer, data))
            except:
                pass

    def sendRequest(self, peer, request, timeout=0, **arguments):
        """
        send request to an existed session.
        :param session:
        :param command:
        :param arguments:
        :param timeout:
        :return:
        """
        setattr(request, "start_time", time.time())
        setattr(request, "timeout", timeout)

        self.sendPayload(peer,
                         command=request.command,
                         cseq=request.cseq,
                         sessionkey=request.session.session_key,
                         arguments=arguments)

        with self.lock:
            self.requests[request.cseq] = request

    def _sendCONNECT(self, peer, secretkey):
        cseq = self.getCSeq()
        request = Request("connect", None, cseq, **{"secretkey": secretkey})
        self.sendRequest(peer, request, self.getConfValue("request_timeout", 5))

    def createSession(self, secretkey, **kwargs):
        pass

    def writeSession(self, session, **kwargs):
        if session.status != Session.CONNECTED:
            raise
        pass

    def readSession(self, session, **kwargs):
        pass

    def closeSession(self, session):
        with session.peer.lock:
            try:
                session.peer.sessions.remove(session)
            except:
                pass

        with self.lock:
            try:
                del self.sessions[session.sessionKey]
                del session
            except:
                pass

    def openPeer(self, address, async=False, **options):
        sock_fd = self.proto.socket(address, **options)
        peer = Peer(sock_fd, address)
        peer.async = async

        if peer.async:
            with self.lock:
                self.epoll.register((peer.sock_fd.fileno(), select.EPOLLIN | select.EPOLLPRI | select.EPOLLERR))

        return peer

    def acceptPeer(self, peer, async=False):
        """
        accept a new peer.
        :param local_fd:
        :return:
        """

        if self.proto == UDP:
            new_fd, _ = self.proto.accept(peer.sock_fd)
            return peer.get()
        elif self.proto == TCP:
            new_fd, (host, port) = self.proto.accept(peer.sock_fd)
            new_peer = Peer(new_fd, (host, port))
            new_peer.async = async

            if new_peer.async:
                with self.lock:
                    self.epoll.register(new_peer.sock_fd.fileno(), select.EPOLLIN | select.EPOLLPRI | select.EPOLLERR)
                    self.remote_peers[new_peer.sock_fd.fileno()] = new_peer
        else:
            raise ProtoUnsupportedException()

        return new_peer

    def readPeer(self, peer):
        with peer.lock:
            data = self.proto.receive(peer.sock_fd)
            peer.last_read_time = time.time()

        return data

    def writePeer(self, peer, data):
        with peer.lock:
            self.proto.send(peer.sock_fd, data)
            peer.last_write_time = time.time()

    def connectPeer(self, peer, address=None):
        with peer.lock:
            self.proto.connect(peer.sock_fd, address)
            peer.last_connect_time = time.time()
            return True

    def getCSeq(self):
        with self.lock:
            self.cseq += 1

            return self.cseq

    def createSession(self, peer, address, secretkey, connect_timeout=5):
        cseq = self.getCSeq()

        self.connectPeer(peer, address)
        self.sendPayload(peer, command="connect", cseq=cseq, arguments={"secretkey": secretkey})
        self.sendRequest()
        session = Session()
        session.status = Session.CONNECTING
        peer.sessions[]
        if peer.async:
            self.requests[self.cseq] = cseq

        session.status = Session.CONNECTING
        self.sendPayload(peer, cseq=)

        while True:

        while retry > 0 and retry_times < retry:
            try:
                self.connectPeer(peer, address)
            except:
                time.sleep(interval)
                retry_times += 1
                continue

            _ = select.select((), (), (), 1.0)

    def closePeer(self, peer):
        """
        close remote connection.
        :param peer:
        :return:
        """

        with peer.lock:
            peer.refcount -= 1
            if peer.refcount > 0:
                return
            else:
                sessions = peer.sessions

        for s in sessions:
            self.closeSession(s)

        with self.lock:
            try:
                self.epoll.unregister(peer.sock_fd.fileno())
            except:
                pass
            try:
                del self.remote_peers[peer.sock_fd.fileno]
                del peer
            except:
                pass

                # try:
                #     peer.sock_fd.close()
                #         except:
                #             pass
                #     try:
                #         del peer
                #     except:
                #         pass

                # self.lock.acquire()
                # try:
                #     try:
                #         self.epoll.unregister(peer.sock_fd.fileno())
                #     except Exception as e:
                #         pass
                #
                #     for s in peer.sessions:
                #         self.closeSession(s)
                #         peer.removeSess
                #     # remove from local collections
                #     try:
                #         del self.connections[0][peer.sock_fd.fileno()]
                #     except:
                #         pass
                #
                #     try:
                #         self.connections[1].remove(peer)
                #     except:
                #         pass
                # finally:
                #     self.lock.release()
                #
                # try:
                #     self.closeSession(peer.session)
                #     peer.sock_fd.close()
                #     Log.i(__package__,
                #           event='disconnect',
                #           object=self.__class__.__name__,
                #           remote_host=peer.address[0],
                #           remote_port=peer.address[1])
                # finally:
                #     peer.lock.release()
                #
                # del peer

    def processIncoming(self, peer):
        """
        read from remote connection and process data.
        :param peer:
        :return:
        """

        data = peer.read(self.proto)
        if self.proto == TCP and len(data) <= 0:
            raise IOError("")

        with self.lock:
            self.write_task('IN', Task(self._handle_in, peer, self.payload.payload(data)))

    def waitForReply(self, request, timeout=0):
        if not hasattr(request, '_sync_lock'):
            raise ReferenceError("")

        if not request._sync_lock.acquire(True, timeout):
            return None

        if hasattr(request, 'response'):
            resp = getattr(request, 'response')
        else:
            resp = None
        try:
            del request._sync_lock
            del request
        except:
            pass

        return resp

    def sendReply(self, request, errcode, **arguments):
        self.sendPayload(request.session.peer,
                         errcode=errcode,
                         cseq=request.cseq,
                         sessionkey=request.session.session_key,
                         arguments=arguments)

    def _handle_in(self, peer, payload):
        peer.lock.acquire()
        event = None

        try:
            event, cseq, sessionkey, errcode, args = self.payload.process(payload)
            if event is None:
                with self.lock:
                    request = self.requests.pop(cseq, None)
                if request:
                    response = Response(request, errcode, args)
                    if hasattr(request, "_sync_lock"):
                        request.response = response
                        try:
                            request._sync_lock.release()
                        except:
                            pass
                    else:
                        try:
                            if self.listener:
                                self.listener.onResponseArrived(request.session, response)
                            del request
                            del response
                        except:
                            pass
                else:
                    pass
            elif event == "connect":
                secretkey = args.get('secretkey')
                sessionkey = self.listener.onAuthenticate(peer, secretkey)
                if not sessionkey:
                    self.sendPayload(peer, cseq=cseq, errcode="ERR_AUTHEN_FAILED")

                session = self.createSession(peer, sessionkey, secretkey)
                if not session:
                    self.sendPayload(peer, cseq=cseq, errcode="ERR_SYSTEM_EXCEPTION")

                # session.secret_key = secretkey
                self.sendPayload(peer,
                                 cseq=cseq,
                                 errcode="ERR_SUCCESS",
                                 sessionkey=sessionkey,
                                 arguments={"ping_interval": self.wait_timeout[0]})
            elif event == "disconnect":
                session = self.getSession(sessionkey)
                if session:
                    self.closeSession(session)
            elif event == "ping":
                session = self.sessions.get(sessionkey)
                if session:
                    self.listener.onPing(session, **args)
                    self.sendPayload(peer,
                                     cseq=cseq,
                                     errcode="ERR_SUCCESS",
                                     sessionkey=sessionkey)
                else:
                    self.sendPayload(peer,
                                     cseq=cseq,
                                     errcode="ERR_SESSION_NOT_FOUND")
            else:
                session = self.sessions.get(sessionkey)
                if not session:
                    self.sendPayload(peer, cseq=cseq, errcode="ERR_SESSION_NOT_FOUND")
                else:
                    self.listener.onRequest(session, Request(event, session, cseq, **args))
        except Exception as e:
            # logging.error("Closing connection due to %s." % (e.message))
            traceback.print_exc()
            Log.e(__package__,
                  event=event,
                  object=self.__class__.__name__,
                  result="failure",
                  reason=e.message)
        finally:
            peer.lock.release()

    def _handle_out(self, peer, data):
        try:
            peer.sock_fd.send(data)
        except Exception as e:
            Log.e(__package__, event='send', log=e.message)
            self.closePeer(peer)

    def _watch_dog(self):
        pass

    def __init__(self,
                 address,
                 proto=TCP,
                 listener=Listener,
                 payload=JsonPayload,
                 **options):

        in_queue_size, in_num_thrds = options.get("in_queue", (100, 1))
        out_queue_size, out_num_thrds = options.get("out_queue", (100, 1))

        super(SessionManager, self).__init__(TaskQueue("IN", in_queue_size, in_num_thrds),
                                             TaskQueue("OUT", out_queue_size, out_queue_size))
        self.listener = listener()
        self.payload = payload()
        self.proto = proto

        self.listener.manager = self
        self.address = address
        self._exit_flag = False
        self.lock = threading.Lock()
        self.requests = {}
        self.cseq = 0
        self.counter = 0
        self.options = options

        self.peer = Peer(self.proto.socket(address, **options), address, self.closePeer)
        self.epoll = select.epoll()
        self.epoll.register(self.peer.sock_fd.fileno(), select.EPOLLIN | select.EPOLLPRI | select.EPOLLERR)
        self.scheduler = Scheduler()

        self.scheduler.register("watch_dog", options.get("keepalive", 1), self.watch_dog)
        # self.sock_fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM, socket.IPPROTO_IP)
        # self.sock_fd.setblocking(False)
        # self.sock_fd.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, True)
        # self.sock_fd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, True)
        # self.sock_fd.bind(self.address)
        # self.sock_fd.listen(500)

        self.remote_peers = {}
        self.status = SessionManager.DISCONNECTED

        self.sessions = {}
        self.connections = [{}, []]
        self.session_list = []
        self.scheduler = Scheduler()
        pass

    # create new session.
    def createSession(self, peer, sessionkey, secretkey):
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
        self.lock.acquire()
        session = None
        try:
            session = Session(peer, sessionkey)
            session.secret_key = secretkey
            if peer.session:
                self.closeSession(peer.session)

            peer.session = session
            self.sessions[sessionkey] = session
            if self.listener:
                self.listener.onConnect(session)
            return session
        except Exception as e:
            if session:
                self.closeSession(session)
            Log.e(__package__,
                  event="open",
                  object=self.__class__.__name__,
                  result='failure',
                  reason=e.message)
            return None
        finally:
            Log.i(__package__,
                  event='open',
                  object=self.__class__.__name__,
                  status='success',
                  session_key=session.session_key)
            self.lock.release()

    def getSession(self, session_key):
        """
        return an existed session.
        if no session found, None will be returned.
        :param session_key:
        :return:
        """
        with self.lock:
            return self.sessions.get(session_key)

    def closeSession(self, session):
        """
        close an existed session.
        :param session:
        :return:
        """
        if not session:
            return

        Log.i(__package__,
              event='close',
              object=self.__class__.__name__,
              session_key=session.session_key)

        self.lock.acquire()
        try:
            if self.listener:
                self.listener.onDisconnect(session)

            if self.sessions.has_key(session.session_key):
                del self.sessions[session.session_key]

            session.peer.session = None
            del session
        finally:
            self.lock.release()

    def _watch_dog(self):
        # import time
        while not self._exit_flag:
            time.sleep(1)
            self.counter += 1

            if self.config.wait_timeout[0] > 0 and self.counter % self.config.wait_timeout[0] == 0:
                with self.lock:
                    peers = self.connections[1]

                for peer in peers:
                    with peer.lock:
                        expired_seconds = self.counter - peer.last_update

                    if expired_seconds > self.config.wait_timeout[0] * self.config.wait_timeout[1]:
                        Log.i(__package__,
                              event="read",
                              object=self.__class__.__name__,
                              result="failure",
                              reason="timed out")
                        self.closePeer(peer)
                    else:
                        break

            with self.lock:
                keys = self.requests.keys()

            for r in keys:
                self.lock.acquire()
                try:
                    request = self.requests.get(r)
                    if request and request.timeout > 0 and self.counter - request.start_time > request.timeout:
                        if self.listener:
                            self.listener.onResponseTimeout(request.session, request)
                        Log.i(__package__,
                              event="request",
                              object=self.__class__.__name__,
                              command=request.command,
                              cseq=request.cseq,
                              session_key=request.session.session_key,
                              reason='request timed out')
                        del self.requests[r]
                        del request
                finally:
                    self.lock.release()

    def _receive_remote(self):
        ev_fds = self.epoll.poll(1.0, maxevents=-1)
        if len(ev_fds) < 0:
            return

        for fd, ev in ev_fds:
            if ev & select.EPOLLIN:
                if fd == self.peer.sock_fd.fileno():
                    self.acceptPeer(self.peer)
                else:
                    peer = self.remote_peers[fd]
                    self.processIncoming(peer)
            else:
                raise IOError("")

    def _connect_remote(self):
        self.lock.acquire()
        try:
            if len(self.peer.sessions) > 0:
                session = self.peer.sessions.pop(0)
            else:
                session = Session()

    def start(self):
        """
        start session manager.
        :return:
        """
        super(SessionManager, self).start()
        self.scheduler.start()

        while not self._exit_flag:
            try:
                ev_fds = self.epoll.poll(1.0, maxevents=-1)
            except IOError:
                continue

            if len(ev_fds) <= 0:  # if timedout
                continue
            else:
                for fd, ev in ev_fds:
                    try:
                        if ev & select.EPOLLIN:
                            if fd == self.sock_fd.fileno():
                                self.acceptPeer(self.sock_fd)
                            else:
                                self.processPeer(fd)
                        elif ev & select.EPOLLERR or ev & select.EPOLLHUP:
                            logging.error("EPOLLERR or EPOLLHUP event occurred.")
                            continue
                        else:
                            pass
                    except Exception as e:
                        logging.error(e.message)
                        Log.i(__package__,
                              event='read',
                              object=self.__class__.__name__,
                              result="failure",
                              reason=e.message)
                        self.closePeer(fd)

        self.scheduler.stop()

    def stop(self, *args):
        super(SessionManager, self).stop()

        self._exit_flag = True
        self.scheduler.stop()
        logging.info("The session manager is exiting.")


def start_default_manager(address, listener):
    def sig_proc(sig, f):
        sessionManager.stop()

    logging.basicConfig(level=logging.INFO)

    sessionManager = SessionManager(address, listener)
    signal.signal(signal.SIGTERM, sig_proc)
    signal.signal(signal.SIGINT, sig_proc)
    signal.signal(signal.SIGPIPE, signal.SIG_IGN)
    signal.signal(signal.SIGHUP, signal.SIG_IGN)
    sessionManager.start()
