/* Copyright (c) 2003-2004 
 * Stefano Ceccherini <burton666@libero.it>. All rights reserved.
 * This file is released under the MIT license
 */
 
#ifndef __WB_DRIVER_H
#define __WB_DRIVER_H

// PCI Communications
#include <PCI.h>

#define IO_PORT_PCI_ACCESS true
//#define MEMORY_MAPPED_PCI_ACCESS true

#if IO_PORT_PCI_ACCESS
#	define write8(address,value)		(*gPci->write_io_8)((address),(value))
#	define write16(address,value)		(*gPci->write_io_16)((address),(value))
#	define write32(address,value)		(*gPci->write_io_32)((address),(value))
#	define read8(address)				((*gPci->read_io_8)(address))
#	define read16(address)				((*gPci->read_io_16)(address))
#	define read32(address)				((*gPci->read_io_32)(address))
#else	/* MEMORY_MAPPED_PCI_ACCESS */
#	define read8(address)   			(*((volatile uint8*)(address)))
#	define read16(address)  			(*((volatile uint16*)(address)))
#	define read32(address) 			(*((volatile uint32*)(address)))
#	define write8(address,data)  		(*((volatile uint8 *)(address))  = data)
#	define write16(address,data) 		(*((volatile uint16 *)(address)) = (data))
#	define write32(address,data) 		(*((volatile uint32 *)(address)) = (data))
#endif

extern pci_module_info* gPci;

#endif
