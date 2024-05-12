/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */

#include "RPC_Print.h"

bool_t
print_user_op_1_svc(PrintArgs arg1, void *result,  struct svc_req *rqstp)
{
    printf("%s %s %s %s\n", arg1.User_name, arg1.Op_name, arg1.File_name, arg1.Time);
    return TRUE;
}

int
print_prog_1_freeresult (SVCXPRT *transp, xdrproc_t xdr_result, caddr_t result)
{
	xdr_free (xdr_result, result);
	return 1;
}
