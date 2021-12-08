
#!/bin/bash

if [ $# -lt 2 ];then
	printf "########Usage %s file1 file2\n" $0
	exit
fi

#mkdir rsa_key

cat $1 > file
cat $2 >> file

#echo "Generate RSA private key in PEM PKCS#1 format"
#openssl genrsa -F4 2048 > rsa_key/myprivkey.pem

#echo "Get mincrypt public key info - len, n, rr, n0inv"
#python pem_extract_pubkey.py rsa_key/myprivkey.pem > rsa_key/rsa2048_pkey_1.h

echo "To sign $@ and generate a signature"
#openssl dgst -sha256 -binary file > rsa_key/file.sha 
openssl dgst -sha256 -sign rsa_key/myprivkey.pem -out rsa_key/signature2048_1 file

xxd -i -c16 rsa_key/signature2048_1 > rsa_key/signature2048_1.h

rm -rf file