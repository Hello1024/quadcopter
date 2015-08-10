Buspirate control code
----------------------

This lets you use a buspirate direct from a PC to control an XN297.   It doesn't work well due to the PC not being able to time TX and RX slots well enough.

Use with:

    gcc bind.c   buspirate.c  nrf24l01.c

or

    gcc bind.c  buspirate_binary.c  nrf24l01.c


The binary version uses a much faster interface, allows multiple in flight operations and has less debugging output.
