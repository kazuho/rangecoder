from setuptools import setup, Extension

setup(
	name="range_coder",
	version="1.0",
	description="A fast implementation of a range coder",
	packages=["range_coder"],
	package_dir={"range_coder": "python"},
	license="BSD",
	ext_modules=[
		Extension(
			"range_coder._range_coder",
			language="c++",
			include_dirs=["."],
			sources=[
				"python/src/range_coder_interface.cpp",
				"python/src/module.cpp"
			])])
