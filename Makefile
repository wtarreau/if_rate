all: bin/if_rate

clean:
	rm -f bin/if_rate a.out core src/*.o

bin/if_rate: src/if_rate.o
	mkdir -p bin
	$(CC) -o $@ $<

