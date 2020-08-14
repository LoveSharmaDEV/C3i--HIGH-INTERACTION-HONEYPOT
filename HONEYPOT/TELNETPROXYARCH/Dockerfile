FROM python:latest

MAINTAINER lovesharmadev1998@gmail.com

RUN mkdir TELNETPROXY

RUN pip install twisted

# SPECIFY AND ENVIRONMENT VARIABLE NAMED 'REMOTE_IP_ADDRESS'
COPY telnetproxy.py ./TELNETPROXY/telnetproxy.py

COPY banner.dat     ./TELNETPROXY/banner.dat

ENTRYPOINT python3 /TELNETPROXY/telnetproxy.py
