#!/bin/sh -e


if [ -z "$PASV_ADDRESS" ]; then
  echo "** This container will not run without setting for PASV_ADDRESS **"
  sleep 10
  exit 1
fi



if ( id ${FTPUSER_NAME} ); then
  echo "User ${FTPUSER_NAME} already exists"
else
  echo "Creating user ${FTPUSER_NAME}"
  ENC_PASS=$(perl -e 'print crypt($ARGV[0], "password")' ${FTPUSER_PASSWORD_SECRET})
  useradd -d /ftp/${USER} -m -p ${ENC_PASS} -u 1000 -s /bin/sh ${FTPUSER_NAME}
fi

mkdir -p /ftp && chown ftpuser /ftp



sed -i \
    -e "s:{{ ALLOW_OVERWRITE }}:$ALLOW_OVERWRITE:" \
    -e "s:{{ ANONYMOUS_DISABLE }}:$ANONYMOUS_DISABLE:" \
    -e "s:{{ ANON_UPLOAD_ENABLE }}:$ANON_UPLOAD_ENABLE:" \
    -e "s:{{ LOCAL_UMASK }}:$LOCAL_UMASK:" \
    -e "s:{{ MAX_CLIENTS }}:$MAX_CLIENTS:" \
    -e "s:{{ MAX_INSTANCES }}:$MAX_INSTANCES:" \
    -e "s:{{ PASV_MAX_PORT }}:$PASV_MAX_PORT:" \
    -e "s:{{ PASV_MIN_PORT }}:$PASV_MIN_PORT:" \
    -e "s+{{ SERVER_NAME }}+$SERVER_NAME+" \
    -e "s:{{ WRITE_ENABLE }}:$WRITE_ENABLE:" \
    /etc/proftpd/proftpd.conf

exec proftpd --nodaemon -c /etc/proftpd/proftpd.conf
