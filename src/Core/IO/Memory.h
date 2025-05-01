#ifndef MEMORY_H
#define MEMORY_H

/* FUNCTIONS */
void *MemCPY(void *dest, const void *src, size_t n);
void *MemSet(void *s, int c, size_t n);
void *MemMove(void *dest, const void *src, size_t n);
int MemCMP(const void *s1, const void *s2, size_t n);

#endif