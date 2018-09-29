#!/bin/bash
openssl genrsa -des3 -out ca_privkey.pem 8192
openssl req -x509 -new -nodes -key ca_privkey.pem -sha256 -days 5000 -out ca_root.pem
openssl genrsa -out privkey.pem 8192
openssl req -new -key privkey.pem -out privkey.csr
openssl x509 -req -in privkey.csr -CA ca_root.pem -CAkey ca_privkey.pem -CAcreateserial -out cert.pem -days 5000 -sha256 
