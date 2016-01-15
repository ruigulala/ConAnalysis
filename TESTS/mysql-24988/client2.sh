#!/bin/bash
while true; do echo 'select count(*) from t1;' ; done | ~/Workspace/ConAnalysis/concurrency-exploits/mysql-24988/mysql-install-clang/bin/mysql -f -u bugtest -h 127.0.0.1 db1 > /dev/null
