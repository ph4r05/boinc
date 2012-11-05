
PROGS = cpu_time

all: $(PROGS)


cpu_time: cpu_time.o  
	gcc -o cpu_time cpu_time.o


clean:
	rm -rf boinc_finish_called boinc_lockfile init_data.xml stderr.txt out *.o checkpoint.txt
