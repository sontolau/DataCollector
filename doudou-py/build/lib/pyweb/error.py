class Err(Exception):
    def __init__(self, message=None):
        self.errcode = self.__class__.__name__
        self.message = message


class ErrLoginRequired(Err):
    pass


class ErrPermissionDenied(Err):
    pass
