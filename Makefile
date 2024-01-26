TARGET1 = iserver
SRCS1 = indexserver.c

TARGET2 = p2pserver
SRCS2 = p2pserver.c

TARGET3 = P2Pclient
SRCS3 = p2pclient.c

TARGET4 = file_info.txt

CC = gcc

all: $(TARGET1)
all: $(TARGET2)
all: $(TARGET3) 

iserver:
$(TARGET1): $(SRCS1)
	$(CC) $(CFLAGS) $(SRCS1) -o $(TARGET1)

p2pserver:
$(TARGET2): $(SRCS2)
	$(CC) $(CFLAGS) $(SRCS2) -o $(TARGET2)

p2pclient:
$(TARGET3): $(SRCS3)
	$(CC) $(CFLAGS) $(SRCS3) -o $(TARGET3)

cleani:
	rm -f $(TARGET1)
	rm -f $(TARGET4)

cleans:
	rm -f $(TARGET2)

cleanc:
	rm -f $(TARGET3)	
