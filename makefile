install:
	python setup.py install --home=.

sdist:
	python setup.py sdist --formats=gztar,zip

SRC=c/eval.c c/mgenerator.c c/search.c

chess.so:
	cc -O -fPIC -shared -o build/chess.so $(SRC)





