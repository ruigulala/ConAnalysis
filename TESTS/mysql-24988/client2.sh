#!/bin/bash
while true; do echo 'select count(*) from t1;' ; done | mysql-install/bin/mysql -f -u bugtest -h 127.0.0.1 db1 > /dev/null
