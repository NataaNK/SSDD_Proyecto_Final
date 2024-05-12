/*
    Autores: Natalia Rodríguez Navarro (100471976)
             Arturo Soto Ruedas (100472007)

    Módulo que implementa la función imprimir operación
    del usuario en el sevidor RPC.
*/

#include "RPC_Print.h"

bool_t
print_user_op_1_svc(PrintArgs arg1, void *result,  struct svc_req *rqstp)
{
    // Imprimir operación por pantalla
    printf("%s %s %s %s\n", arg1.User_name, arg1.Op_name, arg1.File_name, arg1.Time);
    return TRUE;
}

int
print_prog_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);
	return 1;
}
