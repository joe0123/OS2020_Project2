dd if=/dev/urandom of=0.in bs=32M count=10
split -d -n 5 0.in "1.in"
split -d -n 10 0.in "2.in"
