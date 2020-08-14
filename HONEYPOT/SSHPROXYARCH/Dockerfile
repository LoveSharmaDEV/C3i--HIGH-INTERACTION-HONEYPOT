FROM python:latest

MAINTAINER lovesharma <lovesharmadev1998@gmail.com>


# Installing required packages
RUN apt-get update 
RUN apt-get install -y net-tools
RUN apt-get -y install vim
RUN pip install paramiko
RUN apt-get install -y supervisor


RUN mkdir PROXIES
RUN mkdir ./PROXIES/keys
RUN mkdir ./PROXIES/logs

COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY Mod_sshpot.py ./PROXIES
COPY stop-supervisor.sh ./PROXIES
RUN chmod +x ./PROXIES/stop-supervisor.sh
COPY keys /PROXIES/keys
COPY logs /PROXIES/logs

#ENTRYPOINT /usr/bin/supervisord

ENTRYPOINT python /PROXIES/Mod_sshpot.py
