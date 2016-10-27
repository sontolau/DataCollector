import logging

import time


class Log(object):
    @staticmethod
    def to_string(tag, **kwargs):
        info = None
        for k in kwargs.keys():
            if info is not None:
                info = '{0},{1}=\"{2}\"'.format(info, str(k), kwargs[k])
            else:
                info = '{0}=\"{1}\"'.format(str(k), kwargs[k])

        info = tag+"\t"+info
        info += '\n'

        return info

    @staticmethod
    def i(tag, **kwargs):
        logging.info(Log.to_string(tag, **kwargs))

    @staticmethod
    def e(tag, **kwargs):
        logging.error(Log.to_string(tag, **kwargs))

    @staticmethod
    def w(tag, **kwargs):
        logging.warning(Log.to_string(tag, **kwargs))

    @staticmethod
    def d(tag, **kwargs):
        logging.debug(Log.to_string(tag, **kwargs))
