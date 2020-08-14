FROM ubuntu:latest
MAINTAINER lovesharma <lovesharmadev1998@gmail.com>

# Installing required packages
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update 

# INSTALLING RADIUS OLDEST VERSION
RUN apt-get install -y devscripts quilt debhelper fakeroot equivs
RUN  apt-get install -y libssl-dev libtalloc-dev libkqueue-dev
COPY freeradius-server-release_3_0_18.zip ./
RUN apt-get install -y unzip
RUN apt-get install -y make
RUN apt-get install -y gcc
RUN apt-get install -y software-properties-common 
RUN unzip freeradius-server-release_3_0_18.zip
WORKDIR freeradius-server-release_3_0_18
RUN ./configure --enable-developer 
RUN   make
RUN   make install 

RUN apt-get install -y freeradius-mysql
RUN apt-get install -y vim
RUN apt-get install -y mysql-client

#ENV DB_HOST
#ENV DB_PORT
#ENV DB_USER
#ENV DB_PASS
#ENV DB_NAME

ADD clients.conf /usr/local/etc/raddb/clients.conf
ADD users /usr/local/etc/raddb/users



COPY sql /usr/local/etc/raddb/mods-enabled/sql
COPY radiusd.conf  /usr/local/etc/raddb/radiusd.conf
RUN mv /usr/lib/freeradius/rlm_sql_mysql.so  /usr/local/lib/rlm_sql_mysql.so

RUN mkdir sqlschema
COPY radius.sql  /sqlschema/radius.sql

COPY init.sh ./init.sh
RUN chmod +x ./init.sh
EXPOSE 1812/udp
EXPOSE 1813/udp

ENTRYPOINT ./init.sh
#ENTRYPOINT bash
#ENTRYPOINT radiusd -X
# mysql --host=172.17.0.3 --user=root --password=74YeOD@1 radius

