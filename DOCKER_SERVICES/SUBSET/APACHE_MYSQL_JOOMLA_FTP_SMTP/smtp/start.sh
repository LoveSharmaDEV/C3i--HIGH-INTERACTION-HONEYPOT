#!/bin/bash



if ( id ${USER} ); then
  echo "User ${USER} already exists"
else
  echo "Creating user ${USER}"
  ENC_PASS=$(perl -e 'print crypt($ARGV[0], "password")' ${PASS})
  useradd -d /smtp/${USER} -m -p ${ENC_PASS} -u 1000 -s /bin/sh ${USER}
fi


