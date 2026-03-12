#!/bin/bash

rm *.h
rm *.c
rm Makefile*

#rpcgen -a -C -N -M isp_rpc.x
rpcgen -a -C -N isp_rpc.x

sed '5i #pragma GCC diagnostic push\n#pragma GCC diagnostic ignored \"-Wunused-variable\"' -i isp_rpc_xdr.c
sed '$a #pragma GCC diagnostic pop\n' -i isp_rpc_xdr.c
