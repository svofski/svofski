avrdude -p m644 -c pony-stk200 -P lpt1 -U flash:w:"stkload_m644_2k64k_18432000.hex":i -v -V -u