FROM python:latest

MAINTAINER lovesharma <lovesharmadev1998@gmail.com>


# Installing required packages
RUN apt-get update 
RUN apt-get install -y net-tools
RUN apt-get -y install vim
RUN apt-get install -y supervisor


RUN mkdir PROXIES
RUN mkdir ./PROXIES/logs

COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY FTP_PROXY.py ./PROXIES
COPY stop-supervisor.sh ./PROXIES
RUN chmod +x ./PROXIES/stop-supervisor.sh
COPY logs /PROXIES/logs
COPY init.sh /PROXIES/init.sh
RUN chmod +x /PROXIES/init.sh

#ENTRYPOINT /usr/bin/supervisord

ENTRYPOINT /PROXIES/init.sh
