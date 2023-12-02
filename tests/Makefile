

DIR_SRC := ../
DIRS_INCLUDE := $(DIR_SRC) ./

#######################################

SOURCES_C := 
# SOURCES_C := $(filter-out $(DIR_SRC)/systeminit.c, $(SOURCES_C))
SOURCES_C += test.c
SOURCES_C += $(wildcard $(DIR_SRC)/*.c)

# no .d files will be genearated, we will monitor the headers
INCLUDES_H := $(wildcard $(DIR_SRC)/*.h)
INCLUDES_H += $(wildcard *.h)

TARGET_PE := test

#######################################
FLAGS_C := 
FLAGS_C += $(addprefix -I,$(DIRS_INCLUDE))
#FLAGS_C += -std=gnu11
# FLAGS_C += -O3
FLAGS_C += -Wall -Wpedantic

#######################################
$(TARGET_PE): .force_remake

$(TARGET_PE): $(SOURCES_C) $(INCLUDES_H) .force_remake
	echo "# will be overwritten by Make" > .gitignore
	echo ".gitignore" >> .gitignore
	echo $(TARGET_PE) >> .gitignore
	gcc $(FLAGS_C) $(SOURCES_C) -o $@
	./$(TARGET_PE)
# gcc $(FLAGS_C) $^ -o $@
# @echo "gcc : $(notdir $@)"

.force_remake:

clean:
	rm -rf $(TARGET_PE)


