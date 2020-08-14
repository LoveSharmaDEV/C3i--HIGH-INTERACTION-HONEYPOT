FROM fluent/fluentd:latest

MAINTAINER LoveSharmaDev <lovesharmadev1998@gmail.com>

COPY fluentd.conf  /etc/td-agent/fluentd.conf

ENTRYPOINT fluentd -c /etc/td-agent/fluentd.conf
