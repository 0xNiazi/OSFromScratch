ENTRY (_start)

SECTIONS
{ 
	/* Set the mem location coutner to 0x1000 */
	. = 0x1000;

	/* Code sec */
	.text : ALIGN(4) {
		*(.text)
	}

	/* Read-Only data ( string literals, constants) */
	.rodata : ALIGN(4) {
		*(.rodata*)
	}

	/* Initilaized global data */
	.data : ALIGN(4) {
		*(.data)
	}
	
	/* Uninitilized global data */
	.bss : ALIGN(4) {
		*(COMMON)
		*(.bss)
	}
}

