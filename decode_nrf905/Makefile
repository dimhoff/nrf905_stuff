all: decode_nrf905

decode_nrf905: decode_nrf905.c
	gcc decode_nrf905.c lib_crc.c -o decode_nrf905

nrf905_demod.py:
	grcc -d . nrf905_demod.grc

clean:
	rm -f decode_nrf905
