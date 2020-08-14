FROM ubuntu:18.04
MAINTAINER Aman Ahuja "amanjiofficial@gmail.com"

RUN apt-get update
RUN apt-get -y install build-essential && apt-get -y install sudo
COPY openssl-1.0.2u.tar.gz ./
RUN tar -zxvf openssl-1.0.2u.tar.gz
COPY node-v9.8.0.tar.gz ./
RUN tar -zxvf node-v9.8.0.tar.gz
WORKDIR openssl-1.0.2u
RUN ./config
RUN make
RUN sudo make install
RUN apt-get -y install libz-dev python
WORKDIR ../node-v9.8.0
RUN ./configure
RUN make
RUN sudo make install
RUN npm install mysql
RUN npm install http
COPY app.js ./
EXPOSE 8080
CMD node app.js $http_db_host $http_db_user $http_db_pass

