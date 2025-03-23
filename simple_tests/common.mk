# clang-16 is not working with opencilk, get "clang: error: unknown argument: '-fopencilk'"
CC := clang

ifeq ($(DEBUG),1)
# Previously -march=x86-64
	CFLAGS += -O0 -g -gdwarf-4 -fopencilk 
else
# Previously -march=x86-64
	CFLAGS += -Wall -O3 -g -gdwarf-4 -fopencilk 
endif

ifeq ($(LOCAL),1)
	CFLAGS += -march=native
else
# You will be graded on znver2.
	CFLAGS += -march=znver2
endif

ifeq ($(SERIAL),1)
	CFLAGS += -DSERIAL
endif

LDFLAGS := -fopencilk -ldl -lm -lstdc++

DEPS := $(DEPS) ../fasttime.h ../common.mk Makefile

# Original LDFLAGS had -static as well, doesn't seem to work for HPC2
# It appears pthreads is not available as a static library on HPC2
ifeq ($(CILKSAN),1)
	CFLAGS += -fsanitize=cilk -DCILKSAN=1
	LDFLAGS += -fsanitize=cilk
else ifeq ($(CILKSCALE),1)
	CFLAGS += -fcilktool=cilkscale -DCILKSCALE=1
#	LDFLAGS += -fcilktool=cilkscale -static
	LDFLAGS += -fcilktool=cilkscale
else
#	LDFLAGS += -static
endif



.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC) $(DEPS)
	$(CC) -o $@ $(CFLAGS) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
