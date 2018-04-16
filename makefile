#HaiJing20171128
SUBDIRS	= libSysCommon libtinyXml bnlogTask monitorTask

all:$(SUBDIRS)

$(SUBDIRS):ECHO
		make -C $@
		
ECHO:
		@echo $(SUBDIRS)
		
		
.PRONY:clean
clean:
	@echo "Removing linked and compiled files......"
	find . -name "*.o" | xargs rm -f
	find . -name "*.so" | xargs rm -f
	rm -f bnlogTask/bnlog
	rm -f monitorTask/monitor

