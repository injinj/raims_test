FROM quay.io/centos/centos:stream8

RUN rpm -Uvh https://dl.fedoraproject.org/pub/epel/epel-release-latest-8.noarch.rpm
RUN yum -y install telnet iputils iproute net-tools redhat-lsb-core pcre2-devel liblzf-devel make gcc-c++ git openssl-devel chrpath zlib-devel libbsd-devel which vim gdb c-ares-devel rpm-build byacc
RUN rpm -i http://www.rai.technology/raicache-2.0.0.127-RH7_x86_64_MTN_VIEW_REL_API_220811.x86_64.rpm
RUN curl -s https://www.raitechnology.com/dev/raitechnology_epel-8-x86_64.repo -o /etc/yum.repos.d/raitechnology_epel-8-x86_64.repo
RUN yum -y install raims

