import logging


class Log(object):
    @staticmethod
    def to_string(**kwargs):
        info = ''
        for k in kwargs.keys():
            if len(info) > 0:
                info = '{0} {1}=\"{2}\"'.format(info, str(k), kwargs[k])
            else:
                info = '{0}=\"{1}\"'.format(str(k), kwargs[k])
        info += '\n'

        return info

    @staticmethod
    def i(**kwargs):
        logging.info(Log.to_string(**kwargs))

    @staticmethod
    def e(**kwargs):
        logging.error(Log.to_string(**kwargs))


    @staticmethod
    def w(**kwargs):
        logging.warning(Log.to_string(**kwargs))

    @staticmethod
    def d(**kwargs):
        logging.debug(Log.to_string(**kwargs))