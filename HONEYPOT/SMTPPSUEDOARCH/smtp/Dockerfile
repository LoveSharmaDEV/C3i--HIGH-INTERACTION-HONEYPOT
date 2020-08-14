FROM ubuntu:18.04
MAINTAINER c3iintern


RUN apt-get update
RUN apt-get -y install build-essential && apt-get -y install sudo libpcre3 libpcre3-dev libdb-dev libxt-dev libxaw7-dev
RUN sudo groupadd -g 31 exim
RUN sudo useradd -d /dev/null -c "Exim Daemon" -g exim -s /bin/false -u 31 exim
COPY exim-4.92.tar.gz ./
RUN tar -zxvf exim-4.92.tar.gz
WORKDIR exim-4.92
COPY eximon.conf ./Local
COPY Makefile ./Local
RUN sudo make
RUN sudo make install


USER root
RUN chmod 1777 /var/mail
RUN echo 'root:root' | chpasswd


RUN  adduser --disabled-password --gecos '' smtpuser1
RUN echo 'smtpuser1:smtpuser1' | chpasswd
RUN usermod -aG sudo smtpuser1

RUN  adduser --disabled-password --gecos '' smtpuser2
RUN echo 'smtpuser2:smtpuser2' | chpasswd
RUN usermod -aG sudo smtpuser2

RUN  adduser --disabled-password --gecos '' smtpuser3
RUN echo 'smtpuser3:smtpuser3' | chpasswd
RUN usermod -aG sudo smtpuser3

# INSTALLING SYSLOG
RUN apt-get install -y rsyslog
RUN   echo "*.* @127.0.0.1:5140" >> /etc/rsyslog.conf


# INSTALLING TD_AGENT
RUN apt-get -y install curl libcurl4-openssl-dev ruby ruby-dev make
RUN apt-get install -y gnupg
RUN  curl https://packages.treasuredata.com/GPG-KEY-td-agent | apt-key add -
RUN echo "deb http://packages.treasuredata.com/2/ubuntu/trusty/ trusty contrib" > /etc/apt/sources.list.d/treasure-data.list
RUN apt-get update
RUN apt-get install -y td-agent

COPY tail.conf  /etc/td-agent/tail.conf
COPY start.sh  /start.sh

RUN chmod +x /start.sh


EXPOSE 25
COPY exim.conf /etc/exim.conf
WORKDIR /
ENTRYPOINT ./start.sh
#CMD bash
#CMD /usr/sbin/exim -bd -q15m
#service rsyslog start

