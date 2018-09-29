#!/bin/sh
make -C src clean killgch
cd certs
rm -f *.pem *.csr *.srl
./genkeys.sh
cd ..
rm -f src/server/*.pem
OSVAL=`echo $(uname -s) | tr "[:upper:]" "[:lower:]"`
echo $OSVAL
make -C src utilities
make -C src trinity OS=$OSVAL CPU=`uname -m` SERVER=127.0.0.1 AUTHTOKEN=`./src/utils/genauthtoken` IDENTITY=dawn ROOTCERT=../../certs/ca_root.pem DEBUG=1 IPV6=1

cp certs/privkey.pem src/server/serverprivatekey.pem
cp certs/cert.pem src/server/servercert.pem

echo "ServerName=localhost" >> src/control/control.conf
echo "ScriptsDirectory=../../scripts" >> src/control/control.conf
echo "DownloadDirectory=/tmp" >> src/control/control.conf
echo "RootCertificate=../../certs/ca_root.pem" >> src/control/control.conf
