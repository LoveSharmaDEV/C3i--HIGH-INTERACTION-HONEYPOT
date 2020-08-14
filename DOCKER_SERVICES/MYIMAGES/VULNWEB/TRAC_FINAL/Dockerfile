FROM python:2.7

MAINTAINER LoveSharmaDev <lovesharmadev1998@gmail.com>

# UPDATE REPOSITORY
RUN apt-get update


# INSTALLING APACHE2 SERVER
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y apache2 

# INSTALLING TRAC AND DEPENDENCIES
COPY Trac-0.11.1.tar.gz /
RUN tar -xvzf Trac-0.11.1.tar.gz
RUN cd Trac-0.11.1 && python setup.py install
#RUN pip install trac
RUN apt-get install libapache2-mod-wsgi -y

# ENABLE AUTH_DIGEST MODULE
RUN a2enmod auth_digest

# SETTING ARCHITECTURE
RUN mkdir /var/lib/trac
RUN mkdir -p /var/www/html/trac
RUN chown www-data:www-data /var/www/html/trac
RUN apt-get install -y vim
RUN apt-get install -y libmariadb-dev python-dev
RUN pip install mysql-connector-python
RUN apt-get install libmariadbclient-dev
RUN pip install mysql-connector-python


#RUN echo "test \n mysql://root:74YeOD@1@172.17.0.2/trac\n\n" | /usr/local/bin/trac-admin /var/lib/trac/test initenv
RUN echo "test \n sqlite:db/trac.db\n\n" | /usr/local/bin/trac-admin /var/lib/trac/test initenv
ENTRYPOINT tracd --port 8000 /var/lib/trac/test
#ENTRYPOINT bash
