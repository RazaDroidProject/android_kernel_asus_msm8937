OBJS = bbm.o fc8350.o fc8350_bb.o fc8350_i2c.o fc8350_isr.o fc8350_spi.o fc8350_tun.o fc8350_tun_table.o fci_hal.o fci_hpi.o fci_oal.o fci_ringbuffer.o fci_tun.o

EXTRA_CFLAGS += -I$(PWD)/../inc
KDIR = /home/mv4412/src/mv4412-kernel-3.0.15
ODIR = $(PWD)/../

obj-m = isdbt.o
isdbt-objs = $(OBJS)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	mv *.ko			$(ODIR)./
	mv *.o			$(ODIR)
	mv isdbt.mod.c		$(ODIR)
	mv Module.symvers	$(ODIR)
	mv modules.order	$(ODIR)
	mv .*.o.cmd		$(ODIR)
	mv .*.ko.cmd		$(ODIR)
	rm -rf $(ODIR)*.o
	rm -rf $(ODIR)isdbt.mod.c
	rm -rf $(ODIR)Module.symvers
	rm -rf $(ODIR)modules.order
	rm -rf $(ODIR).*.o.cmd
	rm -rf $(ODIR).*.ko.cmd


clean:
	rm -rf $(ODIR)*.ko
	rm -rf $(ODIR)*.o
	rm -rf $(ODIR)isdbt.mod.c
	rm -rf $(ODIR)Module.symvers
	rm -rf $(ODIR)modules.order
	rm -rf $(ODIR).*.o.cmd
	rm -rf $(ODIR).*.ko.cmd
