CFLAGS=-Wall -O3 -g -Wextra -Wno-unused-parameter
CXXFLAGS=$(CFLAGS)

# Where our library resides. You mostly only need to change the
# RGB_LIB_DISTRIBUTION, this is where the library is checked out.
RGB_LIB_DISTRIBUTION=matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a
LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread -lncurses


osc: osc.o
	$(CXX) $(CXXFLAGS) osc.o -o $@ -lasound $(LDFLAGS) $(RGB_LDFLAGS)

osc.o: osc.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) osc.cc -c -o $@ -lasound $(LDFLAGS) $(RGB_LDFLAGS) 

cosc: cosc.o
	$(CXX) $(CXXFLAGS) cosc.o -o $@ -lasound $(LDFLAGS) $(RGB_LDFLAGS)

cosc.o: cosc.cc
	$(CXX) -I$(RGB_INCDIR) $(CXXFLAGS) cosc.cc -c -o $@ -lasound $(LDFLAGS) $(RGB_LDFLAGS) 