install:
	python setup.py install --home=.

sdist:
	python setup.py sdist --formats=gztar,zip
