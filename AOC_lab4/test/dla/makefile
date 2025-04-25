CASES = dla0 dla1 dla2 dla3

all:
	for case in $(CASES); do \
		cd $$case && $(MAKE) all; \
		cd -; \
	done

clean:
	for case in $(CASES); do \
		cd $$case && $(MAKE) clean; \
		cd -; \
	done

test:
	for case in $(CASES); do \
		cd $$case && $(MAKE) test; \
		cd -; \
	done
