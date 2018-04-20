CXX       ?= g++ -g -ggdb 
CXXFLAGS  += -I $(GCCLOCAL)/include -march=native -pthread
LINKFLAGS += -L $(GCCLOCAL)/lib 
LIBS      += -lboost_program_options -lboost_filesystem -lboost_system -lpthread

DEST = dblpbibtex
OBJS = dblpbibtex.o
HEADERS =

all: $(DEST)
clean:
	rm -f $(DEST) $(OBJS)
proper: clean
	rm -f *~

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(DEST): $(OBJS)
	$(CXX) $(LINKFLAGS) -o $@ $< $(LIBS) $(LIBS)


