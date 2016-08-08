# MySQL-24988 Testing w/ TSAN Build Tips
 * Install sysbench: `sudo apt-get install sysbench`
 * Create symlink: `sudo ln -s /tmp/mysql.sock /var/run/mysqld/mysqld.sock
 * Create directory: `mkdir output`
 * Run test by typing `./sysbench`.  Output should be in `output/tsan.[PID]`

