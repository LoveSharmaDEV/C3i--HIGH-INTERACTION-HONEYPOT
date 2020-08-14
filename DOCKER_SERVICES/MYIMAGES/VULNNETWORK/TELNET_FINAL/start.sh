#!/bin/bash



if ( id ${USER} ); then
  echo "User ${USER} already exists"
else
  echo "Creating user ${USER}"
  ENC_PASS=$(perl -e 'print crypt($ARGV[0], "password")' ${PASS})
  useradd -d /ftp/${USER} -m -p ${ENC_PASS} -u 1000 -s /bin/sh ${USER}
fi

/etc/init.d/xinetd restart
service rsyslog start
service ssh start

td-agent -c /etc/td-agent/tail.conf



