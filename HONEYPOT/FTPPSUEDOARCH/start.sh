#!/bin/bash
if ( id ${USER} ); then
  echo "User ${USER} already exists"
else
  echo "Creating user ${USER}"
  ENC_PASS=$(perl -e 'print crypt($ARGV[0], "password")' ${PASS})
  useradd -d /ftp/${USER} -m -p ${ENC_PASS} -u 1000 -s /bin/sh ${USER}
fi


iptables -A INPUT -p tcp -m tcp --dport 30020:30021 -j ACCEPT
echo "iptable added"



service rsyslog start
echo "Service Syslog has started"

proftpd -c /etc/basic.conf
echo "Service PROFTP has started"


echo "Initiating td-agent for tailing logs to stdout"
td-agent -c /etc/td-agent/tail.conf
