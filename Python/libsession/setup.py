try:
    from setuptools import setup
except:
    from distutils.core import setup
from libsession import __version__, __author__, __author_mail__

setup(name='libsession',
      version=__version__,
      author=__author__,
      author_email=__author_mail__,
      platforms=['Unix-based OS'],
      packages=['libsession'])
