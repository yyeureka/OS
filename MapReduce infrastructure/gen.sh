mkdir /tmp/input
dd if=/dev/urandom bs=3M count=1 | base64 > /tmp/input/testdata_1.txt
touch /tmp/input/testdata_2.txt
dd if=/dev/urandom bs=5M count=1 | base64 > /tmp/input/testdata_3.txt
truncate --size=-1 /tmp/input/testdata_3.txt
