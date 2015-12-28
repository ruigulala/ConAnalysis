#!/bin/bash
while true; do mysql-install/bin/mysql -u root -e "FLUSH PRIVILEGES;"; done
