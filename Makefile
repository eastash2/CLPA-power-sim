all: 
	$(MAKE) -C src all
	mv src/sim ./
clean:
	$(MAKE) -C src clean
	rm -f ./sim
