FROM ubuntu:18.04
MAINTAINER c3iintern

RUN apt-get update
RUN apt-get -y install build-essential && apt-get -y install sudo
COPY openssh-7.3p1.tar.gz ./
RUN tar -zxvf openssh-7.3p1.tar.gz
COPY openssl-1.0.2u.tar.gz ./
RUN tar -zxvf openssl-1.0.2u.tar.gz
WORKDIR openssl-1.0.2u
RUN ./config
RUN make
RUN sudo make install
RUN apt-get -y install libz-dev
RUN mkdir /var/lib/sshd
RUN sudo chmod -R 700 /var/lib/sshd/
RUN sudo chown -R root:sys /var/lib/sshd/
RUN sudo useradd -r -U -d /var/lib/sshd/ -c "sshd privsep" -s /bin/false sshd && echo "sshd:amanji" | chpasswd
RUN apt-get -y install libpam0g-dev libselinux1-dev
WORKDIR ../openssh-7.3p1
RUN ./configure --with-pam --with-selinux --with-privsep-path=/var/lib/sshd --sysconfdir=/usr/local/etc
RUN make
RUN sudo make install
RUN echo 'root:root' | chpasswd
RUN sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin yes/' /usr/local/etc/sshd_config
RUN sed -ri 's/UsePAM yes/#UsePAM yes/g' /usr/local/etc/sshd_config
RUN apt-get update
RUN apt-get install -y vim
EXPOSE 22

CMD ["/usr/local/sbin/sshd", "-D"]
