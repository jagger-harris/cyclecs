# Cyclecs Documentation

This project uses the following documentation toolchain:

```text
Doxygen -> XML -> Breathe -> Sphinx -> HTML
```

Documentation is generated directly through CMake. The build system automatically creates a Python virtual environment and installs the required documentation dependencies.

## Dependencies

* Python >=3.10
* CMake >=3.21
* Doxygen

## Building Documentation

Configure the project:
```shell
cmake -S . -B build -DCLS_BUILD_DOCS=ON
```

Generate the documentation:
```shell
cmake --build build --target docs
```

## Documentation Output

The generated HTML documentation is located in:
```text
build/docs/sphinx/html/
```

Open `index.html` to view the generated documentation.

## Updating the Documentation

After modifying source code comments, Doxygen configuration, or Sphinx documentation, rebuild the documentation with:
```shell
cmake --build build --target docs
```
