FROM ubuntu:latest

MAINTAINER lovesharmadev1998@gmail.com

RUN apt-get update

RUN apt-get install -y python

RUN apt-get install -y python3-pip

RUN pip3 install aiosmtpd

RUN pip3 install asyncio

RUN apt-get install -y nmap

RUN apt-get install -y net-tools

RUN apt-get install -y vim

RUN mkdir SMTP

COPY mailproxy.py  ./SMTP/mailproxy.py

COPY config.ini  ./SMTP/config.ini

COPY init.sh ./SMTP/init.sh

RUN chmod +x ./SMTP/init.sh

EXPOSE 25


#ENTRYPOINT bash
ENTRYPOINT ./SMTP/init.sh
#ENTRYPOINT python /SMTP/mailproxy.py
