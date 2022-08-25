GCCFLAGS=-fcompare-debug-second -std=gnu++17 -w -O3

SHELL=/bin/bash
PREFIX=/usr/local
EXECS=gpio-fan-controller
GCC?=g++
SRC=gpio-fan-controller.cpp
LIBS=-pthread
LDLIBS=-lgpiod

SERVICE=gpio-fan-controller.service
SERVICEPATH=/etc/systemd/system

none:
	@echo ""
	@echo "Pick one of the options:"
	@echo "make build          - builds the daemon executable"
	@echo "make clean          - cleans build environment"
	@echo "sudo make install   - installs the daemon (you need to build it first!)"
	@echo "sudo make uninstall - removes the daemon"

build:
	${GCC} ${LIBS} ${GCCFLAGS} -o ${EXECS} ${SRC} ${LDLIBS}

clean:
	rm -f ${EXECS}

install:
	cp ${EXECS} $(PREFIX)/sbin
	chown root:root $(PREFIX)/sbin/${EXECS}
	cp ${SERVICE} ${SERVICEPATH}
	chown root:root ${SERVICEPATH}/${SERVICE}
	chmod 644 ${SERVICEPATH}/${SERVICE}
	systemctl daemon-reload
	systemctl enable $(EXECS)
	systemctl start $(EXECS)

uninstall:
	systemctl stop $(EXECS)
	systemctl disable $(EXECS)
	rm ${SERVICEPATH}/${SERVICE}
	systemctl daemon-reload
	systemctl reset-failed
	rm $(PREFIX)/sbin/${EXECS}
