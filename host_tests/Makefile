# Host-side test of the USBi request -> I2C/SPI translation (no hardware needed)
#   make test
test:
	gcc -Wall -Wextra -DUSBI_LOG=1 -I../firmware/USBi -o usbi_test test.c mock.c ../firmware/USBi/usbi_proto.c
	./usbi_test
clean:
	rm -f usbi_test
