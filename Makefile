# source and include directories
SRCPATH=src
INCPATH=include

# sources files
SOURCES=$(SRCPATH)/ping.c $(SRCPATH)/network.c $(SRCPATH)/load.c $(SRCPATH)/error.c $(SRCPATH)/graphics.c $(SRCPATH)/string.c $(SRCPATH)/line.c $(SRCPATH)/router.c $(SRCPATH)/options.c $(SRCPATH)/xmlobject.cpp $(SRCPATH)/graph.cpp $(SRCPATH)/cloud.cpp

# required libraries
LIBS=-lping -lexpat -lgd

# main - input file
MAIN=main.c

# output name
TARGET=netmon

# commands
REMOVE=rm -f

# 
netmon: 
	@echo
	@echo Compiling project...
	g++ -Wall -o $(TARGET) $(SRCPATH)/$(MAIN) $(SOURCES) $(LIBS)
	@echo
	@echo -------------- netmon compiling finished ---------------

clean:
	@echo
	@echo Cleaning project...
	$(REMOVE) $(TARGET)
