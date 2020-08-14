FROM ubuntu:18.04
MAINTAINER amanjiofficial@gmail.com


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


RUN  adduser --disabled-password --gecos '' smtpuser

EXPOSE 25
COPY exim.conf /etc/exim.conf
CMD /usr/sbin/exim -bd -q15m
