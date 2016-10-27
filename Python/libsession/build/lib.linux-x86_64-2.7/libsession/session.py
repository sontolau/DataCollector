import json
import socket
import traceback
from time import time
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

        # if not command:
        #     return None, cseq, sessionkey, errcode, args
        # else:
        return command, cseq, sessionkey, errcode, args

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


class Peer(object):
    def __init__(self, sockfd, address=None):
        self.sock_fd = sockfd
        # self.sessions = []
        self.session = None
        self.last_update = 0
        self.address = address
        self.lock = threading.Lock()


class Session(object):
    def __init__(self, peer, session_key):
        self.session_key = session_key
        self.create_time = time.time()
        self.cseq = 0
        self.peer = peer


class Request(object):
    def __init__(self, command, session, cseq, **arguments):
        self.command = command
        self.session = session
        self.cseq = cseq
        self.arguments = arguments
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
    def sendPayload(self, peer, **kwargs):
        """
        send payload to remote connection.
        :param peer:
        :param kwargs:
        :return:
        """
        data = self.payload.serialize(**kwargs)
        self.write_task("OUT", Task(self._handle_out, peer, data))

    def acceptPeer(self, local_fd):
        """
        accept a new peer.
        :param local_fd:
        :return:
        """
        new_fd, (host, port) = local_fd.accept()
        # logging.info("Connection established: <<<<<<< %s,%d" % (host, port))
        peer = self.newPeer(new_fd)
        if peer is None:
            Log.i(__package__,
                  event='connect',
                  object="session",
                  remote_host=host,
                  remote_port=port,
                  status='failure',
                  reason='resource limitation')
            new_fd.close()
            return None

        peer.address = (host, port)
        self._update_peer(peer)
        self.epoll.register(new_fd.fileno(), select.EPOLLIN | select.EPOLLPRI | select.EPOLLERR)
        Log.i(__package__,
              event='connect',
              object=self.__class__.__name__,
              remote_host=host,
              remote_port=port,
              status='success')
        return peer

    def _update_peer(self, peer):
        self.lock.acquire()
        try:
            self.connections[1].remove(peer)
        except:
            pass

        try:
            self.connections[1].append(peer)
            peer.last_update = self.counter
        except:
            pass
        self.lock.release()

    def newPeer(self, fd, peer=None):
        """
        return an existed peer or create new peer with specific file descriptor.
        :param fd:
        :param peer:
        :return:
        """
        if isinstance(fd, Peer):
            return fd
        elif not isinstance(fd, socket.socket):
            self.lock.acquire()
            try:
                return self.connections[0][fd]
            except:
                if self.max_clients > 0 and len(self.connections[1]) > self.max_clients:
                    logging.error("The server is full, connection will be rejected.")
                    return None

                self.connections[0][fd] = peer
                return peer
            finally:
                self.lock.release()

        else:
            self.lock.acquire()
            try:
                return self.connections[0][fd.fileno()]
            except:
                if self.max_clients > 0 and len(self.connections[1]) > self.max_clients:
                    logging.error("The server is full, connection will be rejected.")
                    return None

                peer = Peer(fd)
                self.connections[0][fd.fileno()] = peer
                return peer
            finally:
                self.lock.release()

    def closePeer(self, peer):
        """
        close remote connection.
        :param peer:
        :return:
        """
        peer = self.newPeer(peer)
        peer.lock.acquire()
        self.lock.acquire()
        try:
            try:
                self.epoll.unregister(peer.sock_fd.fileno())
            except Exception as e:
                pass
            # remove from local collections
            try:
                del self.connections[0][peer.sock_fd.fileno()]
            except:
                pass

            try:
                self.connections[1].remove(peer)
            except:
                pass
        finally:
            self.lock.release()

        try:
            self.closeSession(peer.session)
            peer.sock_fd.close()
            Log.i(__package__,
                  event='disconnect',
                  object=self.__class__.__name__,
                  remote_host=peer.address[0],
                  remote_port=peer.address[1])
        finally:
            peer.lock.release()

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
        peer.lock.acquire()
        try:
            bufdata = peer.sock_fd.recv(self.max_bytes)
            if len(bufdata) <= 0:
                raise IOError("The remote peer has closed the connection.")

            self._update_peer(peer)
            self.write_task('IN', Task(self._handle_in, peer, self.payload.payload(bufdata)))
        except Exception as e:
            raise IOError(e.message)
        finally:
            peer.lock.release()

    def sendRequest(self, session, command, timeout=0, sync=False, **arguments):
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
                          session=session,
                          arguments=arguments)

        setattr(request, "start_time", self.counter)
        setattr(request, "timeout", timeout)
        self.sendPayload(session.peer,
                         command=command,
                         cseq=self.cseq,
                         sessionkey=session.session_key,
                         arguments=arguments)

        with self.lock:
            self.requests[self.cseq] = request
            self.cseq += 1

        if sync:
            request._sync_lock = threading.Lock()
            request._sync_lock.acquire()

        return request

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

    def __init__(self,
                 address,
                 listener=Listener,
                 payload=JsonPayload,
                 in_queue=(100, 1),
                 out_queue=(100, 1),
                 # max_sessions=0,
                 max_clients=0,
                 wait_timeout=(0, 0),
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

            if self.wait_timeout[0] > 0 and self.counter % self.wait_timeout[0] == 0:
                with self.lock:
                    peers = self.connections[1]

                for peer in peers:
                    with peer.lock:
                        expired_seconds = self.counter - peer.last_update

                    if expired_seconds > self.wait_timeout[0] * self.wait_timeout[1]:
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

        self.watch_dog.join(2)
        self.sock_fd.close()
        self.epoll.close()
        super(SessionManager, self).stop()

    def stop(self, *args):
        self._exit_flag = True
        logging.info("The session manager is exiting.")


def start_manager(address):
    def sig_proc(sig, f):
        sessionManager.stop()

    logging.basicConfig(level=logging.INFO)

    sessionManager = SessionManager(address)
    signal.signal(signal.SIGTERM, sig_proc)
    signal.signal(signal.SIGINT, sig_proc)
    signal.signal(signal.SIGPIPE, signal.SIG_IGN)
    signal.signal(signal.SIGHUP, signal.SIG_IGN)
    sessionManager.start()
