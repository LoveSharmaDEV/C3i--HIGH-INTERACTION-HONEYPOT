#!/bin/bash

while true
do
sleep 20
mysql --host=${DB_HOST} --user=${DB_USER} --password=${DB_PASS} radius < /sqlschema/radius.sql

if [ $? == 0 ]
then
echo "Connected to Mysql REMOTE server"
break
fi

done

# docker run -it -e DB_HOST=172.17.0.2 -e DB_PORT=3306 -e DB_USER=root -e DB_PASS=74YeOD@1 -e DB_NAME=radius radiusarch:2 


radiusd -X


