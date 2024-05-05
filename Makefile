local/publish:
	python setup.py bdist_wheel
	pip install dist/pythonic_core-0.1-cp312-cp312-linux_x86_64.whl --force-reinstall