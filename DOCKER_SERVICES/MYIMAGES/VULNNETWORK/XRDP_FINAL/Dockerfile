FROM ubuntu:latest

MAINTAINER Love Sharma <lovesharmadev>

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update

RUN apt-get install -y software-properties-common
RUN apt-get install -y  xfce4 
RUN apt-get install -y xfce4-goodies
RUN apt-get install -y xorg
RUN apt-get install -y dbus-x11 
RUN apt-get install -y x11-xserver-utils


RUN apt-get install -y make
RUN apt-get install -y gcc
RUN apt-get install -y unzip
COPY xrdp-0.9.4.tar.gz ./
RUN tar -xvzf xrdp-0.9.4.tar.gz
WORKDIR  xrdp-0.9.4

RUN groupadd -r xrdp && useradd -r -g xrdp xrdp
RUN apt-get install -y git 
RUN apt-get install -y autoconf
RUN apt-get install -y libtool
RUN apt-get install -y pkg-config
RUN apt-get install -y g++ 
RUN apt-get install -y libssl-dev
RUN apt-get install -y libpam0g-dev 
RUN apt-get install -y libjpeg-dev
RUN apt-get install -y libx11-dev
RUN apt-get install -y libxfixes-dev
RUN apt-get install -y libxrandr-dev
RUN apt-get install -y flex
RUN apt-get install -y bison
RUN apt-get install -y libxml2-dev
RUN apt-get install -y intltool
RUN apt-get install -y xsltproc
RUN apt-get install -y xutils-dev
RUN apt-get install -y python-libxml2
RUN apt-get install -y  xutils 
RUN apt-get install -y libfuse-dev
RUN apt-get install -y libmp3lame-dev
RUN apt-get install -y nasm
RUN apt-get install -y libpixman-1-dev
RUN apt-get install -y xserver-xorg-dev


RUN ./bootstrap
RUN  ./configure --enable-fuse --enable-mp3lame --enable-pixman --enable-sound
RUN make -j2
RUN make install
RUN adduser xrdp ssl-cert
RUN ln -s /usr/local/sbin/xrdp{,-sesman} /usr/sbin

# INSTALLING SUPERVISORD
RUN apt-get install -y supervisor
RUN mkdir -p /var/log/supervisor
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY stop-supervisor.sh /etc/supervisor/conf.d/stop-supervisor.sh
RUN chmod +x /etc/supervisor/conf.d/stop-supervisor.sh

EXPOSE 3389

ENTRYPOINT  xrdp -nodaemon

