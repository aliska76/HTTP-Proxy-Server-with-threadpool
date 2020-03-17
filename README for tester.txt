Tester HTTP Proxy Server

The tester extracts, tests & logs.


PRE:
chmod +x *.sh

USAGE:
1. copy your TAR into the tester's folder (must be of the form ID_111111111_work.tar, e.g. ID_111111111_work.tar)
tar -cvf ID_111111111_work.tar proxyServer.c threadpool.c proxy-files filter.txt
2. run: ./tester_extract.sh ID_111111111_work.tar

Results:
1. the results log file will be generated under the logs folder
2. the tests grades will appear in grades.csv (1 = pass, 0 = fail)
