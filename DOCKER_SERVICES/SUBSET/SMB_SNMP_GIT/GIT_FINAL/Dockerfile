#USING UBUNTU AS BASE IMAGE
FROM ubuntu:18.04

MAINTAINER LoveSharmaDev <lovesharmadev1998@gmail.com>

# UPDATE THE UBUNTU REPOSITORIES
RUN apt-get update

# RUN IN NON-INTERACTIVE MOD FOR ESCAPING PROMPTS
ENV DEBIAN_FRONTEND=noninteractive

# INSTALL GIT PACKAGE
RUN apt-get install -y make
RUN apt-get install -y gcc
RUN apt-get install -y dh-autoreconf libcurl4-gnutls-dev libexpat1-dev gettext libz-dev libssl-dev
RUN apt-get install -y asciidoc xmlto docbook2x
RUN apt-get install -y install-info
COPY git-2.19.0.tar.gz ./
RUN tar -xvzf git-2.19.0.tar.gz
WORKDIR  git-2.19.0
RUN make configure
RUN ./configure --prefix=/usr 
RUN   make all doc info
RUN   make install install-doc install-html install-info


#INSTALL APACHE2 PACKAGE
RUN apt-get  install -y apache2 
RUN apt-get  install -y apache2-utils


# INSTALL OPEN SSH PACKAGE
RUN apt-get install -y openssh-server
RUN mkdir /var/run/sshd
RUN echo 'root:root' | chpasswd
RUN sed -ri 's/^#?PermitRootLogin\s+.*/PermitRootLogin yes/' /etc/ssh/sshd_config
RUN sed -ri 's/UsePAM yes/#UsePAM yes/g' /etc/ssh/sshd_config
RUN mkdir /root/.ssh


# CREATE GIT PROJECT
RUN a2enmod env cgi alias rewrite
RUN mkdir  /var/www/git
RUN chown -Rfv www-data:www-data /var/www/git
COPY git.conf /etc/apache2/sites-available/git.conf
COPY git-create-repo.sh /usr/bin/git-create-repo.sh
RUN chmod +x /usr/bin/git-create-repo.sh
RUN /usr/bin/git-create-repo.sh test




# UPDATE THE SITE CONFIG
RUN a2dissite 000-default.conf
RUN a2ensite git.conf

# SETTING ENVIRONMENT VARIABLE
ENV APACHE_RUN_GROUP www-data
ENV APACHE_LOG_DIR /var/log/apache2
ENV APACHE_LOCK_DIR /var/lock/apache2
ENV APACHE_PID_FILE /var/run/apache2.pid

RUN chmod -R 777 /var/www/git/
# EXPOSING PORTS AND SETTING ENTRYPOINT
EXPOSE 80/tcp
#ENTRYPOINT bash
ENTRYPOINT service apache2 restart && /usr/sbin/sshd -D





