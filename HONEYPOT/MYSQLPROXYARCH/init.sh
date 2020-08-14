#!bin/sh

sed -i 's/<IPADDRESS1>/'$(cat /etc/hostname)'/g' /PROXIES/MYSQL_PROXY.py

echo "RUNNING FTP PROXY......................"\

python /PROXIES/MYSQL_PROXY.py
