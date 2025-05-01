#ifndef GDT_H
#define GDT_H

/* VARIABLES */
extern bool GDTInitialized;


/* FUNCTIONS */
void GDTSetEntry(uint8_t EntryIndex, uint16_t Limit, uint32_t Base, uint8_t Access, uint8_t Gran);
void InitGDT();

#endif