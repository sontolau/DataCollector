import socket


class ProtoUnsupportedException(Exception):
    pass


class Protocol(object):
    @classmethod
    def socket(cls, address, **options):
        raise NotImplementedError()

    @classmethod
    def setOptions(cls, fd, address, **options):
        # fd.setblocking(options.get("blocking", False))
        fd.settimeout(options.get('snd_rcv_timeout', 0))
        fd.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, True)
        fd.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, options.get('snd_buf_size', 0xFFFF))
        fd.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, options.get('rcv_buf_size', 0xFFFF))

        if options.get("bind", False):
            fd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, options.get("reuseaddr", True))
            fd.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, options.get("reuseport", True))
            fd.bind(address)
            fd.listen(options.get("backlog", 100))

    @classmethod
    def connect(cls, fd, host):
        pass

    @classmethod
    def close(cls, fd):
        pass

    @classmethod
    def accept(cls, fd):
        pass

    @classmethod
    def send(cls, fd, data):
        pass

    @classmethod
    def receive(cls, fd):
        pass


class TCP(Protocol):
    @classmethod
    def socket(cls, address, **options):
        if not options.get("is_unix", False):
            fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        else:
            fd = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM, 0)

        cls.setOptions(fd, **options)
        return fd

    @classmethod
    def connect(cls, fd, host):
        return fd.connect(host)

    @classmethod
    def accept(cls, fd):
        fd.accept()

    @classmethod
    def send(cls, fd, data):
        bytes = bytearray(data)
        fd.send(bytes)

    @classmethod
    def receive(cls, fd):
        return fd.recv(0xFFFF)


class UDP(Protocol):
    @classmethod
    def socket(cls, address, **options):
        if not options.get('is_unix', False):
            fd = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, 0)
        else:
            fd = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM, 0)

        cls.setOptions(fd, address, **options)

        setattr(fd, "target", address)
        return fd

    @classmethod
    def connect(cls, fd, host):
        return fd

    @classmethod
    def accept(cls, fd):
        return fd

    @classmethod
    def send(cls, fd, data):
        bytes = bytearray(data)
        fd.sendto(bytes, addr=fd.target)

    @classmethod
    def receive(cls, fd):
        return fd.recvfrom(0xFFFF)
