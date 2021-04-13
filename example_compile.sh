#!/bin/sh
set -e


if ! test -f certs/cert.pem; then
	cd certs
	./genkeys.sh
	cd ..
fi

rm -f src/server/*.pem
OSVAL=`echo $(uname -s) | tr "[:upper:]" "[:lower:]"`
echo $OSVAL
cd src/utils
rm -rf build
mkdir -p build
cd build
cmake ..
make -j
cd ../../server
rm -rf build
mkdir -p build
cd build
cmake .. -DOS=$OSVAL -DCPU=`uname -m` -DDEBUG=1 -DIPV6=1
make -j
cd ../../node
rm -rf build
mkdir -p build
cd build
cmake .. -DOS=$OSVAL -DCPU=`uname -m` -DSERVER=127.0.0.1 -DAUTHTOKEN=`../../utils/build/genauthtoken` -DIDENTITY=dawn -DCERT=../../../certs/cert.pem -DDEBUG=1 -DIPV6=1 -DLUAFFI=ON
make -j
cd ../../control
rm -rf build
mkdir -p build
cd build
cmake .. -DOS=$OSVAL -DCPU=`uname -m` -DDEBUG=1 -DIPV6=1
make -j
cd ../../../

cp certs/privkey.pem src/server/serverprivatekey.pem
cp certs/cert.pem src/server/servercert.pem

echo "ServerName=localhost" >> src/control/control.conf
echo "ScriptsDirectory=../../scripts" >> src/control/control.conf
echo "DownloadDirectory=/tmp" >> src/control/control.conf
echo "RootCertificate=../../certs/ca_root.pem" >> src/control/control.conf
