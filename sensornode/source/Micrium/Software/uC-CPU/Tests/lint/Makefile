LINT=lint-nt
LINTFLAGS=\
-I. \
-I.. \
-I../.. \
-I../../Template \
-I../../../uC-LIB \
-I../../../uC-LIB/Cfg/Template \
-IC:\lint\lnt \
-IC:\lint \
-U__cplusplus \
-fff
LINT_ARG=std.lnt include.lnt env-keil.lnt micrium.lnt
EXECUTABLE=hello
.PHONY: hello
all: $(EXECUTABLE)
	$(LINT) $(LINTFLAGS) $(LINT_ARG) project.lnt
