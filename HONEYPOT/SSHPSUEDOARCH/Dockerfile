FROM python:latest

MAINTAINER lovesharma <lovesharmadev1998@gmail.com>


# Installing and configuring SSH service
RUN apt-get update 
RUN apt-get install -y supervisor
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

RUN apt-get install -y supervisor

COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf


# Exporting essential ports
EXPOSE 22

# Defining Entry Points

ENTRYPOINT  /usr/sbin/sshd -D


