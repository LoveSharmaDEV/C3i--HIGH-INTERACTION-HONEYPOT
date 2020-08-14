#!bin/sh

sed -i 's/<IPADDRESS1>/'$(ifconfig eth0 | sed -En 's/.*inet ([0-9.]+).*/\1/p')'/g' ./VULNWEB/VulnWeb.py

echo "RUNNING HTTP PROXY......................"\

python3 ./VULNWEB/VulnWeb.py
