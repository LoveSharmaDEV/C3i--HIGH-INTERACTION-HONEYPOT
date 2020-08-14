FROM python:latest

MAINTAINER lovesharmadev1998@gmail.com



RUN mkdir VULNWEB

RUN apt-get update 

RUN apt-get install -y nmap

RUN apt-get install -y net-tools

COPY VulnWeb.py ./VULNWEB/VulnWeb.py



COPY init.sh ./VULNWEB/init.sh

RUN chmod +x ./VULNWEB/init.sh

EXPOSE 55555

ENTRYPOINT ./VULNWEB/init.sh
#ENTRYPOINT python /VULNWEB/VulnWeb.py


