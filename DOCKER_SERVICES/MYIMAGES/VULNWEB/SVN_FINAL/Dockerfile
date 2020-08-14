# USING UBUNTU AS THE BASE IMAGE
FROM ubuntu:latest
MAINTAINER LoveSharmaDev <lovesharmadev1998@gmail.com>


# UPDATE THE SYSTEM REPOSITORIES
RUN apt-get update 

# INSTALL APACHE2 SERVER
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y apache2 

# INSTALL SUBVERSION AND DEPENDENCIES
RUN apt-get install -y unzip
RUN apt-get install -y make
RUN apt-get install -y gcc
RUN apt-get install -y software-properties-common 
RUN apt-get install -y autoconf 
RUN apt-get install -y libtool
RUN apt-get install -y python3
RUN apt-get install -y python
RUN apt-get install -y  python3-distutils 



# INSTALL APR
COPY apr-1.6.2.tar.gz ./
RUN tar -xvzf apr-1.6.2.tar.gz
RUN cd apr-1.6.2 &&\
./configure && \
 make all && \
 make install

COPY expat-2.2.9.tar.gz ./
RUN tar -xvzf expat-2.2.9.tar.gz
RUN cd expat-2.2.9 &&\
./configure && \
 make all && \
 make install



# INSTALL APR-UTIL 
COPY apr-util-1.6.1.tar.gz ./
RUN tar -xvzf apr-util-1.6.1.tar.gz
RUN cd apr-util-1.6.1 &&\
./configure -with-apr=/usr/local/apr/bin/apr-1-config && \
 make all && \
 make install

# INSTALL SUBVERSION OLD VERSION
RUN mkdir -p subversion-1.9.1/sqlite-amalgamation
COPY sqlite-amalgamation-3071501 /subversion-1.9.1/sqlite-amalgamation
RUN apt-get install -y libz-dev
COPY subversion-1.9.1.tar.gz ./
RUN tar -xvzf subversion-1.9.1.tar.gz
RUN cd subversion-1.9.1 &&\
./autogen.sh && \ 
./configure -with-apr=/usr/local/apr/bin/apr-1-config  --with-apr-util=/usr/local/apr/bin/apu-1-config && \
 make all && \
 make install

RUN apt-get install -y libsvn-dev 
RUN apt-get install -y libapache2-mod-svn 
RUN a2enmod dav 
RUN a2enmod dav_svn 
RUN service apache2 restart

# CREATING SVN PROJECT
RUN mkdir -p /var/lib/svn/
RUN cd etc && svnadmin create /var/lib/svn/myrepo

RUN chown -R www-data:www-data /var/lib/svn
RUN chmod -R 775 /var/lib/svn   
RUN touch /etc/apache2/dav_svn.passwd
RUN htpasswd -m /etc/apache2/dav_svn.passwd admin
COPY dav_svn.conf /etc/apache2/mods-enabled/dav_svn.conf
COPY svnserve.conf /var/lib/svn/myrepo/conf/svnserve.conf
COPY passwd /var/lib/svn/myrepo/conf/passwd

# INSTALLING SUPERVISOR
RUN apt-get install -y supervisor
RUN mkdir -p /var/log/supervisor
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY stop-supervisor.sh /etc/supervisor/conf.d/stop-supervisor.sh
RUN chmod +x /etc/supervisor/conf.d/stop-supervisor.sh



# DEFINE THE ENTRYPOINT
ENTRYPOINT  service apache2 restart && svnserve -d  --foreground --config-file=/var/lib/svn/myrepo/conf/svnserve.conf

#/usr/bin/supervisord


