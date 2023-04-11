CXX       = g++
RM        = rm -f
CP        = cp

CPPFLAGS  = -I.
CXXFLAGS  = -Wall -Werror -g3 -O0
LDFLAGS   =
LIBS      =

SRCS      = top.cc

BUILD     = build
OBJS      = $(patsubst %.cc, $(BUILD)/%.o, $(SRCS))
DEPS      = $(patsubst %.o,%.d,$(OBJS))
DEPS     += $(patsubst %.cc,%.d,$(GILSRCS))

EXE       = tilegame
FPRODS    =

.PHONY: all
all: $(EXE)


FPRODS += $(EXE)
$(EXE): $(OBJS) Makefile
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

$(OBJS): $(BUILD)/%.o: %.cc
	@mkdir -p `dirname $@`
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -o $@ -c $<

.PHONY: clean
clean:
	$(RM) -R $(BUILD)
	$(RM) $(FPRODS)

