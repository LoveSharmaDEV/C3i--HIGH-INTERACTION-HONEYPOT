#!/bin/bash


mysql --host=${DB_HOST} --user=${DB_USER} --password=${DB_PASS} ${DB_NAME} < /sqlschema/radius.sql

while [ $? -ne 0 ]; do
sleep 10
echo "FAILED TO CONNECT..... NOW RECONNECTING"
mysql --host=${DB_HOST} --user=${DB_USER} --password=${DB_PASS} ${DB_NAME} < /sqlschema/radius.sql
done

radiusd -X
