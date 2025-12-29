
typedef void (*interrupt_fun)(void);

void setvect( unsigned Vector, void  (*NewVector)());
interrupt_fun getvect(unsigned Vector);
void enable(void);
void disable(void);
int inportb(int port);
void outportb(int port, int val);

