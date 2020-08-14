#!bin/sh

sed -i 's/<IPADDR1>/'$(ifconfig eth0 | sed -En 's/.*inet ([0-9.]+).*/\1/p')'/g' ./SMTP/config.ini
sed -i 's/<IPADDR2>/'${REMOTE_IP_ADDRESS}'/g' ./SMTP/config.ini

echo "RUNNING SMTP PROXY......................"

python3 ./SMTP/mailproxy.py
