class Payload(object):
    def payload(self, data, *args, **kwargs):
        """
        this will return an object of payload.
        :param data:
        :param args:
        :param kwargs:
        :return: a payload object.
        """
        return None

    def process(self, payload):
        """
        the handler to process payload data, which must return
        the specific arguments list.
        :param payload:
        :return:command('connect', 'disconnect', 'request', 'response'), cseq, session key, a list of arguments.
        """
        pass

    def serialize(self, **kwargs):
        """
        :param kwargs:
        :return:
        """
        return None
