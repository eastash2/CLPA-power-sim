all: 
	$(MAKE) -C src all
clean:
	$(MAKE) -C src clean
	rm -f ./sim
