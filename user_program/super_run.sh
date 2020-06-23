du -sh *in*
./run_master0.sh m & ./run_slave0.sh m
sleep 1
./diff0.sh
./run_master0.sh m & ./run_slave0.sh f
sleep 1
./diff0.sh
./run_master0.sh f & ./run_slave0.sh m
sleep 1
./diff0.sh
./run_master0.sh f & ./run_slave0.sh f
sleep 1
./diff0.sh
./run_master1.sh m & ./run_slave1.sh m
sleep 1
./diff1.sh
./run_master0.sh f & ./run_slave0.sh m
sleep 1
./diff1.sh
./run_master1.sh m & ./run_slave1.sh f
sleep 1
./diff1.sh
./run_master1.sh f & ./run_slave1.sh m
sleep 1
./diff1.sh
./run_master1.sh f & ./run_slave1.sh f
sleep 1
./diff1.sh
./run_master2.sh m & ./run_slave2.sh m
sleep 1
./diff2.sh
./run_master2.sh m & ./run_slave2.sh f
sleep 1
./diff2.sh
./run_master2.sh f & ./run_slave2.sh m
sleep 1
./diff2.sh
./run_master2.sh f & ./run_slave2.sh f
sleep 1
./diff2.sh
