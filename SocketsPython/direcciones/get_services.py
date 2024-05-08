import socket
from urllib.parse import urlparse

URLS = [
    'http://www.uc3m.es',
    'smtp://mail.gmail.com'
]

for url in URLS:
    parsed_url = urlparse(url)
    port = socket.getservbyname(parsed_url.scheme)
    print('{:>6} : {}'.format(parsed_url.scheme, port))
