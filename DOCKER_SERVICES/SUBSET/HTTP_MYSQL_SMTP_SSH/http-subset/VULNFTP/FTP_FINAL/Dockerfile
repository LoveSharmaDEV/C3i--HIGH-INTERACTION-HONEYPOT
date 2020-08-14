FROM ubuntu:latest

MAINTAINER Love Sharma <lovesharmadev>


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

# INSTALLING SUPERVISOR
RUN apt-get install -y supervisor
RUN mkdir -p /var/log/supervisor
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY stop-supervisor.sh /etc/supervisor/conf.d/stop-supervisor.sh
RUN chmod +x /etc/supervisor/conf.d/stop-supervisor.sh
COPY start.sh /usr/local/bin/start.sh
RUN chmod +x /usr/local/bin/start.sh

#RUN sed -i "s/# DefaultRoot/DefaultRoot/" /etc/proftpd/proftpd.conf

EXPOSE 20 21

ENTRYPOINT /usr/bin/supervisord

#ENTRYPOINT  /usr/bin/supervisord
# /usr/bin/supervisord
