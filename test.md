./schedsim --algorithm=FCFS --input=workload.txt
./schedsim --algorithm=SJF --input=workload.txt
./schedsim --algorithm=STCF --input=workload.txt
./schedsim --algorithm=RR --quantum=50 --input=workload.txt
./schedsim --algorithm=MLFQ --mlfq-config=mlfq_config.txt --input=workload.txt

Compare Mode

./schedsim --compare --input=workload.txt

Validate Error Handling and Exit Codes

./schedsim --algorithm=RR --quantum=0 --input=workload.txt ; echo $?
./schedsim --algorithm=MLFQ --input=workload.txt ; echo $?
./schedsim --algorithm=FCFS --input=does_not_exist.txt ; echo $?