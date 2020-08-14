FROM ubuntu:latest

MAINTAINER Love Sharma <lovesharmadev>

# INSTALLING PROFTPD VULNERABLE VERSION
ENV USER ftpuser
ENV PASS ftpuser
RUN apt-get update 
RUN apt-get install -y make
RUN apt-get install -y gcc
RUN apt-get install -y unzip
COPY proftpd-1.3.5a.zip ./
RUN unzip proftpd-1.3.5a.zip
WORKDIR  proftpd-1.3.5a
RUN make configure
RUN ./configure --prefix=/usr
RUN make all
RUN make install
RUN apt-get clean 
RUN echo 'root:root' | chpasswd
RUN mkdir /ftp
VOLUME ["/ftp"]

#--------------------------------------------------------------------------------------------------------------------------------

# INSTALLING SYSLOG
RUN apt-get install -y rsyslog

#--------------------------------------------------------------------------------------------------------------------------------

# INSTALLING TD_AGENT
RUN apt-get -y install curl libcurl4-openssl-dev ruby ruby-dev make
RUN apt-get install -y gnupg
RUN  curl https://packages.treasuredata.com/GPG-KEY-td-agent | apt-key add -
RUN echo "deb http://packages.treasuredata.com/2/ubuntu/trusty/ trusty contrib" > /etc/apt/sources.list.d/treasure-data.list
RUN apt-get update
RUN apt-get install -y td-agent

#--------------------------------------------------------------------------------------------------------------------------------


# FLUENTD CONFIG FOR TAILING ALL THE LOG FILES TO CONTAINERS STDOUT
COPY tail.conf  /etc/td-agent/tail.conf

#--------------------------------------------------------------------------------------------------------------------------------

# CONFIGURING SYSLOG CONFIG FILE FOR DIRECTING THE LOG TO FLUENTD
RUN   echo "*.* @127.0.0.1:5140" >> /etc/rsyslog.conf

#--------------------------------------------------------------------------------------------------------------------------------


# SCRIPT FOR ENTRY POINT
COPY basic.conf /etc/basic.conf
COPY start.sh /usr/local/bin/start.sh
RUN chmod +x /usr/local/bin/start.sh

#--------------------------------------------------------------------------------------------------------------------------------

RUN apt-get install -y iptables

EXPOSE 20 21 30020 30021


#ENTRYPOINT proftpd --nodaemon
#ENTRYPOINT bash
ENTRYPOINT /usr/local/bin/start.sh
# /usr/bin/supervisord
