FROM python:latest

MAINTAINER lovesharma <lovesharmadev1998@gmail.com>


# Installing required packages
RUN apt-get update 
RUN apt-get install -y net-tools
RUN apt-get -y install vim



RUN mkdir PROXIES
RUN mkdir ./PROXIES/logs


COPY MYSQL_PROXY.py ./PROXIES
COPY logs /PROXIES/logs
COPY init.sh /PROXIES/init.sh
RUN chmod +x /PROXIES/init.sh

#ENTRYPOINT /usr/bin/supervisord

ENTRYPOINT /PROXIES/init.sh
