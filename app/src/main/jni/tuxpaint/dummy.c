// for testing the compiler

#ifdef HEADER
#include <HEADER>
#endif

#define UNUSED __attribute__ ((__unused__))

#ifdef TYPE
static TYPE x UNUSED;
#endif

#ifdef SYMBOL
static int exists UNUSED = !!SYMBOL;
#endif

int main(int argc UNUSED, char *argv[] UNUSED){
	return 0;
}
