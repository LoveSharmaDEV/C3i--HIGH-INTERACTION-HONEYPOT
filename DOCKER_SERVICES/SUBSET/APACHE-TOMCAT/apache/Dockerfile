FROM ubuntu:18.04
MAINTAINER Aman Ahuja "amanjiofficial@gmail.com"

RUN apt-get update
RUN apt-get -y install build-essential && apt-get -y install sudo
COPY httpd-2.4.38.tar.gz ./
RUN tar -zxvf httpd-2.4.38.tar.gz
RUN apt-get install -y libapr1-dev libaprutil1-dev libaprutil1-ldap libpcre3-dev
WORKDIR httpd-2.4.38
RUN ./configure
RUN make
RUN sudo make install
RUN apt-get install -y libapache2-mod-jk

ADD apache2.conf /etc/apache2/apache2.conf

ADD 000-default.conf /etc/apache2/sites-enabled/000-default.conf

ADD worker.properties /etc/libapache2-mod-jk/workers.properties

ADD jk.conf /etc/apache2/mods-available/jk.conf

EXPOSE 80
CMD /usr/local/apache2/bin/apachectl -D FOREGROUND


