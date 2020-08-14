#!bin/sh

sed -i 's/<IPADDRESS1>/'$(cat /etc/hostname)'/g' /PROXIES/FTP_PROXY.py

echo "RUNNING FTP PROXY......................"\

python /PROXIES/FTP_PROXY.py
