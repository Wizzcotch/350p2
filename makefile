# Miguel Lumapat, Denis Plotkin, Alan Plotko
CC = g++
SRCDIR = src
BUILDDIR = build
TARGET = program2
 
SRCEXT := cpp
SOURCES := $(wildcard $(SRCDIR)/*.$(SRCEXT)) #$(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.o))
CPPFLAGS = -Wall -g	-std=c++11
LIB :=
INC := -I include

DRIVE_DIRNAME = DRIVE
SEGMENT_PREFIX = SEGMENT
CHKPTREGION_FILENAME = CHECKPOINT_REGION
FILEASSOCMAP_FILENAME = FILEMAP
COUNT ?= 1

all: $(TARGET) generateDrive

generateDrive:
	@if [ ! -d "$(DRIVE_DIRNAME)" ]; then \
		mkdir -p $(DRIVE_DIRNAME); \
		echo "Set up the DRIVE folder"; \
	fi
	@n=$(COUNT); \
	while [ $${n} -lt 33 ] ; do \
		if [ ! -f $(DRIVE_DIRNAME)/$(SEGMENT_PREFIX)$$n ]; then \
			truncate -s 1M $(DRIVE_DIRNAME)/$(SEGMENT_PREFIX)$$n ; \
		fi ; \
		n=`expr $$n + 1`; \
	done
	@if [ ! -f $(DRIVE_DIRNAME)/$(CHKPTREGION_FILENAME) ]; then \
		truncate -s 192 $(DRIVE_DIRNAME)/$(CHKPTREGION_FILENAME) ; \
		echo "Set up checkpoint region file" ; \
	fi
	@if [ ! -f $(DRIVE_DIRNAME)/$(FILEASSOCMAP_FILENAME) ]; then \
		truncate -s 0 $(DRIVE_DIRNAME)/$(FILEASSOCMAP_FILENAME) ; \
		echo "Set up filename association map file" ; \
	fi

$(TARGET): $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET) $(LIB)"; $(CC) $^ -o $(TARGET) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CPPFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CPPFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

reset:
	@rm -rf DRIVE
