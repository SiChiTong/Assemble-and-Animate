
SOURCES=controller.c
OBJECTS=$(SOURCES:.c=.o)


all:
	@echo Legal targets are "ussr" and "tos" 

tos: 
	avr-gcc -Os -c -DATRON -DTINYOS -Wall -mmcu=atmega128 -I$(ASE_HOME)/include $(SOURCES)
	avr-ar -rcs libControl.a $(OBJECTS)
	mv libControl.a ../../tos/libControl.a 

ussr:
	gcc -c -DATRON -DUSSR -Wall -I$(ASE_HOME)/include $(SOURCES) 
	ar -rcs libControl.a $(OBJECTS) 
	mv libControl.a ../../ussr/libControl.a 

.PHONY: clean
clean:
	rm -f *.o *.out *.gch *.a
