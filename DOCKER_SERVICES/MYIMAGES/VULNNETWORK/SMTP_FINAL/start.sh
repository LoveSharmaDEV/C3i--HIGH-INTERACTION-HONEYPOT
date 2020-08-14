#!/bin/bash

# INITIATING SYSLOG
echo "STARTING SYSLOG"
service rsyslog start

# INITIATING EXIM MAIL SERVER
echo "STARTING MAIL SERVER"
/usr/sbin/exim -bd -q15m

# SENDING DUMMY MAILS 
#exim -v smtpuser1@smtp.c3i.net
#exim -v smtpuser2@smtp.c3i.net
#exim -v smtpuser3@smtp.c3i.net

# INITIATING TDAGNET FOR TAILING PURPOSES
echo "STARTING FLUENTD"
td-agent -c /etc/td-agent/tail.conf
