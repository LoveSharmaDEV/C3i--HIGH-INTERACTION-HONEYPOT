# USING UBUNTU LATEST VERSION AS TEH BASE IMAGE
FROM ubuntu:latest

MAINTAINER LoveSharma <lovesharmadev1998@gmail.com>

# CREATING USER AND GROUPS
RUN groupadd -r jboss -g 1000 && useradd -u 1000 -r -g jboss -m -d /opt/jboss -s /sbin/nologin -c "jboss console user" jboss
RUN apt-get update
RUN apt-get install curl -y
RUN apt-get install wget -y

# Set the working directory to jboss' user home directory
WORKDIR /opt/jboss

# SWITH TO ROOT USER FOR INSTALLING JAVA PACKAGES
USER root
RUN apt-get install default-jdk -y


ENV WILDFLY_VERSION 16.0.0.Final




# Set the JBOSS_HOME env variable
ENV JBOSS_HOME /opt/jboss/wildfly

RUN cd $HOME \
    && curl -O https://download.jboss.org/wildfly/$WILDFLY_VERSION/wildfly-$WILDFLY_VERSION.tar.gz \
    && tar xf wildfly-$WILDFLY_VERSION.tar.gz \
    && mv $HOME/wildfly-$WILDFLY_VERSION $JBOSS_HOME \
    && rm wildfly-$WILDFLY_VERSION.tar.gz \
    && chown -R jboss:0 ${JBOSS_HOME} \
    && chmod -R g+rw ${JBOSS_HOME}


#INSTALL SUPERVISORD
RUN apt-get install -y supervisor
RUN mkdir -p /var/log/supervisor
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
COPY stop-supervisor.sh /etc/supervisor/conf.d/stop-supervisor.sh
RUN chmod +x /etc/supervisor/conf.d/stop-supervisor.sh

# Switch back to jboss user
#USER jboss

RUN /opt/jboss/wildfly/bin/add-user.sh admin admin --silent




EXPOSE 8080 9990


ENTRYPOINT /opt/jboss/wildfly/bin/standalone.sh -c standalone-full.xml -b 0.0.0.0 -bmanagement 0.0.0.0


