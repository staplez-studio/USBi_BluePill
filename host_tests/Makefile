# Host-side tests (no hardware needed):  make test
FW = ../firmware/USBi
test:
	gcc -Wall -Wextra -DUSBI_LOG=1 -I$(FW) -o usbi_test test.c mock.c $(FW)/usbi_proto.c $(FW)/usbi_stats.c
	./usbi_test
	gcc -Wall -Wextra -DUSBI_LOG=0 -I$(FW) -o oled_test oled_test.c mock.c $(FW)/usbi_proto.c $(FW)/usbi_stats.c $(FW)/ui_format.c $(FW)/oled.c
	./oled_test | grep -v '^[OI] rt=\|^  \[wire\]\|^[0-9])'
clean:
	rm -f usbi_test oled_test
