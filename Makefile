CC = gcc
LIBS = zlib libavformat libavcodec libavutil libavfilter
CFLAGS = -O2 -Wall -Wno-deprecated-declarations -Wno-format-extra-args -Wno-unused-function `pkg-config --cflags $(LIBS)`
INC = delogo.h
OBJ = delogo.o crop_delogo.o edge_scan.o hysteresis.o \
      overlap_two.o jumping_logo.o spng.o png_pcx.o
LFLAGS = -lm `pkg-config --libs $(LIBS)`

all:	delogo

$(OBJ): $(INC)

.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

delogo: $(OBJ) 
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	rm -f *~ *.o *.png *.pcx
