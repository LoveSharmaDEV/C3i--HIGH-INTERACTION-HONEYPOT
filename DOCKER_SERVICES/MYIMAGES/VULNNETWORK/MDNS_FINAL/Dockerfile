FROM ubuntu:latest
MAINTAINER LoveSharmaDev <lovesharmadev1998@gmail.com>

# install necessary stuff; avahi, and ssh such that we can log in and control avahi
RUN apt-get update -y
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get install -y make
RUN apt-get install -y gcc
RUN apt-get install -y g++
RUN apt-get install -y intltool
RUN apt-get install -y unzip
COPY avahi-0.6.32.tar.gz ./
RUN tar -xvzf avahi-0.6.32.tar.gz
WORKDIR  avahi-0.6.32

RUN apt-get install -y pkg-config
RUN apt-get install glib2.0 -y
RUN apt-get update
RUN apt-get install  gtk2.0 -y
RUN apt-get install -y gtk3.0
RUN apt-get install -y libgtk-3-dev
RUN apt-get install -y libgdbm-dev
RUN apt-get install -y libdaemon-dev
RUN apt-get install -y 

RUN ./configure --prefix=/usr   --sysconfdir=/etc --localstatedir=/var --disable-static --disable-mono --disable-monodoc --disable-python --disable-qt3 --disable-qt4  --enable-core-docs --with-distro=none --with-systemdsystemunitdir=no  
RUN   make all 
RUN   make install 
#RUN apt-get -qq install -y  avahi-utils 

ADD avahi-daemon.conf /etc/avahi/avahi-daemon.conf

# workaround to get dbus working, required for avahi to talk to dbus. This will be mounted
RUN mkdir -p /var/run/dbus
VOLUME /var/run/dbus

EXPOSE 5353/udp

ENV USER avahi
ENV PASS avahi
WORKDIR /
VOLUME ["/mdns"]
COPY start.sh /usr/local/bin/start.sh
RUN chmod +x /usr/local/bin/start.sh
ENTRYPOINT /usr/local/bin/start.sh


# login keys come in through a volume; we intend to log into root on this to run commands
#ENTRYPOINT avahi-daemon
#ENTRYPOINT  avahi-daemon --no-drop-root
