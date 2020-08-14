# Start with Ubuntu base image
FROM ubuntu:latest
MAINTAINER LoveSharmaDev <lovesharmadev1998@gmail.com>

# Install Samba
RUN apt-get update 

RUN apt-get install -y make
RUN apt-get install -y gcc
RUN apt-get install -y python 
RUN apt-get install -y python-dev
RUN apt-get install -y libkrb5-dev
RUN apt-get install -y krb5-user
RUN apt-get install -y gnutls-bin
RUN apt-get install -y asciidoc xmlto docbook2x
RUN apt-get install -y libacl1-dev
RUN apt-get install -y libldap2-dev
RUN apt install -y rsync grsync
RUN apt-get install -y openssh-server
RUN mkdir /var/run/sshd
RUN echo 'root:root' | chpasswd
RUN sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -ri 's/UsePAM yes/#UsePAM yes/g' /etc/ssh/sshd_config
RUN mkdir /root/.ssh
RUN service ssh start



COPY samba-4.7.2.tar.gz ./
RUN tar -xvzf samba-4.7.2.tar.gz
WORKDIR  samba-4.7.2
RUN make configure
RUN ./configure --without-ad-dc  --without-pam 
RUN make -j
RUN make install


#INSTALL SUPERVISORD
RUN apt-get install -y supervisor
RUN mkdir -p /var/log/supervisor
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY stop-supervisor.sh /etc/supervisor/conf.d/stop-supervisor.sh
RUN chmod +x /etc/supervisor/conf.d/stop-supervisor.sh

COPY smb.conf /usr/local/samba/etc/smb.conf
# Copy config files

COPY smb.txt /

ENV USER root
WORKDIR /

RUN mkdir helloworld
RUN cd helloworld && touch test.txt
  # Adjust Samba configuration
RUN cat smb.txt >> /usr/local/samba/etc/smb.conf 
# Set password

RUN echo "root\nroot\n" | /usr/local/samba/bin/smbpasswd -a root 
#RUN /usr/local/samba/bin/smbpasswd -an root -c /usr/local/samba/etc/smb.conf
  # Make directory to share
RUN mkdir /data 

  # Stop Samba
RUN /usr/local/samba/sbin/smbd 
  # Clean up


# Expose Samba ports
EXPOSE 137 138 139 445
# Run Samba in the foreground

#ENTRYPOINT bash
ENTRYPOINT  /usr/local/samba/sbin/smbd  --foreground --no-process-group 


# smbclient -L 172.17.0.2 -p 139

#smbclient \\\\172.17.0.2\\data 


