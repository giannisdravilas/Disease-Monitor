# # paths
# LIB = ../lib
# INCLUDE = ../include
# MODULES = ../modules

# # compiler
# CC = gcc

# # Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
# CFLAGS = -Wall -Werror -g -I$(INCLUDE)
# LDFLAGS = -lm

# Αρχεία .o
# Αρχεία .o
DiseaseMonitor_test_OBJS = DiseaseMonitor_test.o $(MODULES)/DiseaseMonitor.o $(LIB)/DiseaseMonitorLibToUse.a

# # Το εκτελέσιμο πρόγραμμα
# EXEC = DiseaseMonitor_test

# # Παράμετροι για δοκιμαστική εκτέλεση
# ARGS =

# # Παράμετροι της βιβλιοθήκης raylib
# # include $(LIB)/libraylib.mk


# $(EXEC): $(OBJS)
# 	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

# clean:
# 	rm -f $(OBJS) $(EXEC)

# run: $(EXEC)
# 	./$(EXEC) $(ARGS)



# Ο βασικός κορμός του Makefile
include ../common.mk

$(LIB)/%.a:
	$(MAKE) -C $(LIB) $*.a