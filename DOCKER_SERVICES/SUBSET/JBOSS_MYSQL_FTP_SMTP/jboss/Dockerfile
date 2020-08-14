# USING UBUNTU LATEST VERSION AS TEH BASE IMAGE
FROM ubuntu:18.04

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
RUN mkdir -p ${JBOSS_HOME}/modules/com/mysql/diver/main
WORKDIR ${JBOSS_HOME}/modules/com/mysql/driver/main

COPY mysql-connector-java-8.0.17.jar .
#RUN module add --name=com.mysql.driver  --dependencies=javax.api,javax.transaction.api --resources=mysql-connector-java-8.0.17.tar.gz
COPY ./module.xml .

WORKDIR ${JBOSS_HOME}/standalone/configuration
#RUN rm -r ./standalone/configuration
#ADD  ./standalone/configuration ./standalone
COPY ./standalone-full.xml .
WORKDIR ${JBOSS_HOME}/bin
#WORKDIR ${JBOSS_HOME}/bin

#RUN ./jboss-cli.sh --connect command=:reload
#RUN /subsystem=datasources/jdbc-driver=mysql/:add(driver-module-name=com.mysql.driver,driver-name=mysql,jdbc-compliant=false,driver-class-name=com.mysql.jdbc.Driver)
#RUN /subsystem=datasources/jdbc-driver=mysql/:add(driver-module-name=com.mysql.driver,driver-name=mysql,jdbc-compliant=false,driver-class-name=com.mysql.jdbc.Driver)
USER jboss
RUN /opt/jboss/wildfly/bin/add-user.sh admin admin --silent
EXPOSE 8080 9990
#CMD ["./jboss-cli.sh","--connect","command=:module add --name=com.mysql.driver  --dependencies=javax.api,javax.transaction.api --resources=~/wildfly/modules/system/layers/base/com/mysql/diver/main/mysql-connector-java-8.0.17.jar"]
#CMD ["echo","module add --name=com.mysql.driver  --dependencies=javax.api,javax.transaction.api --resources=~/wildfly/modules/system/layers/base/com/mysql/diver/main/mysql-connector-java-8.0.17.jar","|","./jboss-cli.sh","--connect"]
CMD ["./jboss-cli.sh","--connect","command=:reload"]
ENTRYPOINT /opt/jboss/wildfly/bin/standalone.sh -c standalone-full.xml -b 0.0.0.0 -bmanagement 0.0.0.0


