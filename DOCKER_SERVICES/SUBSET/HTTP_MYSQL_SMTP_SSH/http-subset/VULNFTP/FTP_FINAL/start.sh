#!/bin/bash



if ( id ${USER} ); then
  echo "User ${USER} already exists"
else
  echo "Creating user ${USER}"
  ENC_PASS=$(perl -e 'print crypt($ARGV[0], "password")' ${PASS})
  useradd -d /ftp/${USER} -m -p ${ENC_PASS} -u 1000 -s /bin/sh ${USER}
fi

proftpd

sleep 5
pgrep -x proftpd >/dev/null && VAR=1 || VAR=0

while [ $VAR -eq 1 ]
do
pgrep -x proftpd >/dev/null && VAR=1 || VAR=0
done


