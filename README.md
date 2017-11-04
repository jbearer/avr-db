# avr-db
_An Arduino emulator and debugger_

## Setup

```bash
git clone https://www.github.com/jbearer/avr-db.git
cd avr-db
git submodule init
```

## Example programs

Generate an ELF executable (`<name>.elf`), an AVR hex file (`<name>.hex`), and an assembly code listing (`<name>.lst`):

```bash
cd examples
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

Uploading an example to an AVR board:

```bash
make <name>-flash
```

e.g.

```bash
make blink-flash
```


