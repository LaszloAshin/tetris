#include <SDL.h>

#include <unistd.h>

#include <algorithm>

__attribute__((noreturn))
static void
die(const char* file, int line, const char* expr)
{
	char buffer[256];
	const int n = snprintf(buffer, sizeof(buffer), "%s:%d: assertion failed: %s\n", file, line, expr);
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wunused-result"
	write(STDERR_FILENO, buffer, n);
#	pragma GCC diagnostic pop
	abort();
}

#define CHECK(expr) do { if (!(expr)) die(__FILE__, __LINE__, #expr); } while (0)

struct Deadline {
	unsigned remaining() const { return std::max(0u, value_ - SDL_GetTicks()); }
	bool have_time() const { return value_ > SDL_GetTicks(); }

private:
	uint32_t value_ = SDL_GetTicks() + 40;
};

int
main()
{
	CHECK(SDL_Init(SDL_INIT_VIDEO) >= 0);
	SDL_Window *const window = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
	CHECK(window);
	SDL_Renderer *const rend = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	CHECK(rend);
	bool quit = false;
	int x = 0;
	int dx = 5;
	while (!quit) {
		SDL_Event ev;
		const Deadline dl;
		while (dl.have_time() && SDL_WaitEventTimeout(&ev, dl.remaining())) {
			if (ev.type == SDL_QUIT) {
				quit = true;
			} else if (ev.type == SDL_KEYDOWN) {
				if (ev.key.keysym.sym == SDLK_ESCAPE) {
					quit = true;
				}
			}
		}
		SDL_SetRenderDrawColor(rend, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(rend);
		SDL_SetRenderDrawColor(rend, 255, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderDrawLine(rend, x, 50, 100, 100);
		SDL_RenderPresent(rend);
		x += dx;
		if (x <= 0) {
			x = 1;
			dx = 5;
		} else if (x >= 640) {
			x = 640;
			dx = -5;
		}
	}
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
