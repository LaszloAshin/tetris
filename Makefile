tetris:

.PHONY: clean
clean:
	rm -f tetris *.o

CPPFLAGS += -Wall -Wextra -pedantic -Werror
CXXFLAGS += -std=c++11
CPPFLAGS += -Os -g
CPPFLAGS += $(shell sdl2-config --cflags)
CPPFLAGS += -pthread
#LDLIBS += -Wl,-Bstatic -lSDL2 -lSDL2main -lwayland-client -lwayland-cursor -lXcursor -lX11 -lxcb -lXau -lXdmcp -lXext -lXi -lXinerama -lXrandr -lXss -lXxf86vm -lxkbcommon -lffi -lXrender -lXfixes -Wl,-Bdynamic
#LDLIBS += -lpulse -lpulse-simple -lasound -lwayland-egl -ldl -pthread
LDLIBS += $(shell sdl2-config --libs)
#CPPFLAGS += -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-exceptions
CPPFLAGS += -fdata-sections -ffunction-sections
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Wl,--relax
LDFLAGS += -static-libgcc
LDFLAGS += -static-libstdc++
