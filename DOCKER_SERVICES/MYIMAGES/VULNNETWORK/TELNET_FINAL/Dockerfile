FROM ubuntu:latest

MAINTAINER LoveSharmaDev <lovesharmadev1998@gmail.com>

ENV USER teluser
ENV PASS teluser

RUN apt-get update

RUN apt-get install -y telnetd

RUN apt-get install -y telnet

RUN apt-get install -y  xinetd 


# INSTALLING SUPERVISOR
RUN apt-get install -y supervisor
RUN mkdir -p /var/log/supervisor
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY stop-supervisor.sh /etc/supervisor/conf.d/stop-supervisor.sh
RUN chmod +x /etc/supervisor/conf.d/stop-supervisor.sh
COPY start.sh /usr/local/bin/start.sh
RUN chmod +x /usr/local/bin/start.sh
RUN apt-get install -y inetutils-ping 
RUN apt-get install -y vim
RUN apt-get install -y rsyslog


# INSTALLING TD_AGENT
RUN apt-get -y install curl libcurl4-openssl-dev ruby ruby-dev make
RUN apt-get install -y gnupg
RUN  curl https://packages.treasuredata.com/GPG-KEY-td-agent | apt-key add -
RUN echo "deb http://packages.treasuredata.com/2/ubuntu/trusty/ trusty contrib" > /etc/apt/sources.list.d/treasure-data.list
RUN apt-get update
RUN apt-get install -y td-agent

# INSTALL SSH
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get install -y net-tools
RUN apt-get -y install vim
RUN apt-get install -y openssh-server
RUN mkdir /var/run/sshd
RUN echo 'root:root' | chpasswd
RUN sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -ri 's/UsePAM yes/#UsePAM yes/g' /etc/ssh/sshd_config
RUN mkdir /root/.ssh
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*



# FLUENTD CONFIG
COPY tail.conf  /etc/td-agent/tail.conf
RUN   echo "*.* @127.0.0.1:5140" >> /etc/rsyslog.conf


EXPOSE 23



#ENTRYPOINT /etc/init.d/xinetd restart && bash
#ENTRYPOINT /usr/bin/supervisord
ENTRYPOINT /usr/local/bin/start.sh 
#ENTRYPOINT  bash
