FROM ubuntu:latest
RUN apt-get update; apt-get install snmpd -y
RUN apt-get install snmp -y
RUN apt-get install nano -y

#ADD snmpd.conf /etc/snmp/snmpd.conf
ADD bootstrap.sh /bootstrap.sh
RUN chmod +x /bootstrap.sh

EXPOSE 161:161/udp
EXPOSE 162:162/udp

ENTRYPOINT ["/bootstrap.sh"]

