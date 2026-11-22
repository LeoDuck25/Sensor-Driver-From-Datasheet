.PHONY: test clean
test:
	gcc -Wall -Werror -o test_runner test/test_sensor_math.c main/sensor_math.c -lm 
	./test_runner
clean:
	rm -f test_runner
