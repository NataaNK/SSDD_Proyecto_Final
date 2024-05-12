"""
    Autores: Natalia Rodríguez Navarro (100471976)
             Arturo Soto Ruedas (100472007)

    Servicio web para dar la fecha.
"""

import time
import datetime

from spyne import Application, ServiceBase, Unicode, rpc
from spyne.protocol.soap import Soap11
from spyne.server.wsgi import WsgiApplication

# Exporta application para ser usado en otros módulos.
def get_application():
    return application

class FechaHoraService(ServiceBase):

    @rpc(_returns=Unicode)
    def obtener_fecha_hora(ctx):
        # Obtener la fecha y hora actual y formatearla
        now = datetime.datetime.now()
        return now.strftime("%d/%m/%Y %H:%M:%S")

application = Application(
    services=[FechaHoraService],
    tns='http://tests.python-zeep.org/',
    in_protocol=Soap11(validator='lxml'),
    out_protocol=Soap11())

application = WsgiApplication(application)

if __name__ == '__main__':
    import logging
    from wsgiref.simple_server import make_server

    logging.basicConfig(level=logging.DEBUG)
    logging.getLogger('spyne.protocol.xml').setLevel(logging.DEBUG)

    logging.info("listening to http://127.0.0.1:8000")
    logging.info("wsdl is at: http://localhost:8000/?wsdl")

    server = make_server('127.0.0.1', 8000, application)
    server.serve_forever()
