FROM ubuntu:latest
MAINTAINER LoveSharmaDev "lovesharmadev1998@gmail.com"

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

ENV ALLOW_OVERWRITE=on \
    ANONYMOUS_DISABLE=on \
    ANON_UPLOAD_ENABLE=DenyAll \
    FTPUSER_PASSWORD_SECRET="ftpuser" \
    FTPUSER_NAME="ftpuser" \
    LOCAL_UMASK=022 \
    MAX_CLIENTS=10 \
    MAX_INSTANCES=30 \
    PASV_ADDRESS="127.0.0.1" \
    PASV_MAX_PORT=30100 \
    PASV_MIN_PORT=30091 \
    SERVER_NAME=ProFTPD \
    WRITE_ENABLE=AllowAll

RUN mkdir /etc/proftpd/
COPY proftpd.conf.j2 /etc/proftpd/proftpd.conf

EXPOSE 21 $PASV_MIN_PORT-$PASV_MAX_PORT

COPY entrypoint.sh /usr/local/bin/
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
