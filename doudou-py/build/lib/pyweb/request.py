from django.http import HttpResponseRedirect
from django.views import View
from django.conf.urls import url
from pyweb.error import *


class base_web_class(type):
    pass


def get_web_request_class(meta, *base):
    class base_class(meta):
        def __new__(cls):
            return meta()

    return type.__new__(base_class, 'WebRequestHandler')


class WebRequestHandler(View, get_web_request_class(base_web_class)):
    def error(self, err, request, *args, **kwargs):
        pass

    def handler(self, request, data, files, *args, **kwargs):
        pass

    def get(self, request, *args, **kwargs):
        return self.handler(request, request.GET, request.FILES, *args, **kwargs)

    def post(self, request, *args, **kwargs):
        return self.handler(request, request.POST, request.FILES, *args, **kwargs)


def register_url_handler(regex, view=None, kwargs=None, name=None):
    from .url import urlpatterns
    def proc_func(proc):
        def wrapper(request, *args, **kwargs):
            if issubclass(proc, WebRequestHandler):
                pro = proc()
                if request.method == 'POST':
                    return pro.post(request, *args, **kwargs)
                elif request.method == 'GET':
                    return pro.get(request, *args, **kwargs)
            else:
                raise TypeError(proc.__name__)

        from .url import urlpatterns
        urlpatterns += [url(regex, wrapper)]
        return wrapper

    if view:
        urlpatterns += [url(regex, view, kwargs, name)]
        return

    return proc_func


def login_required_ex(login_url=None):
    def check_login(proc):
        def wrapped(obj, request, *args, **kwargs):
            if not isinstance(obj, WebRequestHandler):
                raise TypeError()
            if request.user is None or not request.user.is_authenticated():
                if login_url is not None:
                    return HttpResponseRedirect(login_url)
                else:
                    return obj.error(ErrLoginRequired(), request)
            return proc(obj, request, *args, **kwargs)

        return wrapped

    return check_login


def permission_required_ex(super_user=False, *perms):
    def validate_permission(proc):
        def wrapped(obj, request, *args, **kwargs):
            if not isinstance(obj, WebRequestHandler):
                raise TypeError()

            if super_user and not request.user.is_admin:
                return obj.error(ErrPermissionDenied(), request)

            if not request.user.has_perms(perms):
                return obj.error(ErrPermissionDenied(), request)
            return proc(obj, request, *args, **kwargs)

        return wrapped

    return validate_permission
