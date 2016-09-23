#include <SDL.h>

#include <algorithm>
#include <memory>
#include <cassert>
#include <stdexcept>

enum class Cell {
	empty, i, o, t, s, z, j, l
};

struct Point {
	int x, y;
};

struct Playfield {
	enum { width = 10, height = 22 };

	Playfield() { cells_.fill(Cell::empty); }

	Cell operator[](Point p) const { return cells_[offset(p)]; }
	Cell& operator[](Point p) { return cells_[offset(p)]; }

private:
	using Cells = std::array<Cell, width * height>;

	static int
	offset(Point p)
	{
		assert(p.x >= 0 && p.x < width);
		assert(p.y >= 0 && p.y < height);
		return p.y * width + p.x;
	}

	Cells cells_;
};

struct Model {
	void
	update()
	{
		pf_[Point{1, 0}] = Cell::i;
		pf_[Point{2, 0}] = Cell::o;
		pf_[Point{3, 0}] = Cell::t;
		pf_[Point{4, 0}] = Cell::s;
		pf_[Point{5, 0}] = Cell::z;
		pf_[Point{6, 0}] = Cell::j;
		pf_[Point{7, 0}] = Cell::l;
	}

	const Playfield& getPlayfield() const { return pf_; }

private:
	Playfield pf_;
};

using SdlWindow = std::unique_ptr<SDL_Window, void(*)(SDL_Window*)>;

struct Sdl {
	Sdl() { if(SDL_Init(SDL_INIT_VIDEO) < 0) throw std::runtime_error("SDL_Init"); }
	Sdl(const Sdl&) = delete;
	Sdl& operator=(const Sdl&) = delete;
	~Sdl() { SDL_Quit(); }

	SdlWindow
	createWindow()
	{
		const auto u = SDL_WINDOWPOS_UNDEFINED;
		SDL_Window* window = SDL_CreateWindow("Tetris", u, u, 10 * 20 + 1, 22 * 20 + 1, SDL_WINDOW_SHOWN);
		if (!window) throw std::runtime_error("SDL_CreateWindow");
		return SdlWindow(window, SDL_DestroyWindow);
	}
};

struct SdlColor {
	uint8_t r, g, b, opacity;

	SdlColor(uint8_t red, uint8_t green, uint8_t blue, uint8_t opacity = SDL_ALPHA_OPAQUE)
	: r(red), g(green), b(blue), opacity(opacity)
	{}

	static SdlColor black() { return SdlColor(0, 0, 0); }
	static SdlColor gray() { return SdlColor(32, 32, 32); }
	static SdlColor cyan() { return SdlColor(0, 255, 255); }
	static SdlColor yellow() { return SdlColor(255, 255, 0); }
	static SdlColor purple() { return SdlColor(128, 0, 128); }
	static SdlColor green() { return SdlColor(0, 255, 0); }
	static SdlColor red() { return SdlColor(255, 0, 0); }
	static SdlColor blue() { return SdlColor(0, 0, 255); }
	static SdlColor orange() { return SdlColor(255, 165, 0); }
};

struct SdlRenderer {
	SdlRenderer(SdlWindow& window)
	: rend_(SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED), SDL_DestroyRenderer)
	{
		if (!rend_) throw std::runtime_error("SDL_CreateRenderer");
	}

	void setDrawColor(SdlColor c) { SDL_SetRenderDrawColor(rend_.get(), c.r, c.g, c.b, c.opacity); }
	void clear() { SDL_RenderClear(rend_.get()); }
	void drawLine(Point a, Point b) { SDL_RenderDrawLine(rend_.get(), a.x, a.y, b.x, b.y); }
	void present() { SDL_RenderPresent(rend_.get()); }
	void fillRect(SDL_Rect r) { SDL_RenderFillRect(rend_.get(), &r); }

private:
	std::unique_ptr<SDL_Renderer, void(*)(SDL_Renderer*)> rend_;
};

struct SdlView {
	SdlView(const Model& model, SdlRenderer& rend) : model_(model), rend_(rend) {}

	void
	render()
	{
		rend_.setDrawColor(SdlColor::black());
		rend_.clear();
		render(model_.getPlayfield());
		rend_.present();
	}

private:
	void
	render(const Playfield& pf)
	{
		const int cellsize = 20;
		rend_.setDrawColor(SdlColor::gray());
		for (int y = 0; y != pf.height + 1; ++y) {
			rend_.drawLine(Point{0, y * cellsize}, Point{pf.width * cellsize, y * cellsize});
		}
		for (int x = 0; x != pf.width + 1; ++x) {
			rend_.drawLine(Point{x * cellsize, 0}, Point{x * cellsize, pf.height * cellsize});
		}
		for (int y = 0; y != pf.height; ++y) {
			for (int x = 0; x != pf.width; ++x) {
				rend_.setDrawColor(getColor(pf[Point{x, y}]));
				rend_.fillRect(SDL_Rect{x * cellsize + 1, y * cellsize + 1, cellsize - 1, cellsize - 1});
			}
		}
	}

	SdlColor
	getColor(Cell c)
	{
		switch (c) {
		case Cell::empty: return SdlColor::black();
		case Cell::i: return SdlColor::cyan();
		case Cell::o: return SdlColor::yellow();
		case Cell::t: return SdlColor::purple();
		case Cell::s: return SdlColor::green();
		case Cell::z: return SdlColor::red();
		case Cell::j: return SdlColor::blue();
		case Cell::l: return SdlColor::orange();
		}
		assert(!"invalid cell");
	}

	const Model& model_;
	SdlRenderer& rend_;
};

struct SdlEvents {
	struct iterator {
		iterator(SdlEvents* se) : se_(se) {}

		bool operator!=(const iterator& rhs) const { return se_ != rhs.se_; }
		iterator& operator++() { assert(se_); se_ = se_->next(); return *this; }
		SDL_Event& operator*() { assert(se_); return se_->ev_; }

	private:
		SdlEvents* se_;
	};

	iterator begin() { return iterator(next()); }
	iterator end() { return iterator(nullptr); }

private:
	struct Deadline {
		unsigned remaining() const { return std::max(0u, value_ - SDL_GetTicks()); }
		bool have_time() const { return value_ > SDL_GetTicks(); }

	private:
		uint32_t value_ = SDL_GetTicks() + 40;
	};

	bool hasNext() { return dl_.have_time() && SDL_WaitEventTimeout(&ev_, dl_.remaining()); }
	SdlEvents* next() { return hasNext() ? this : nullptr; }

	const Deadline dl_;
	SDL_Event ev_;
};

static void
requestSdlQuit()
{
	SDL_Event e;
	e.type = SDL_QUIT;
	e.quit.timestamp = SDL_GetTicks();
	SDL_PushEvent(&e);
}

int
main()
{
	Sdl sdl;
	auto window = sdl.createWindow();
	SdlRenderer rend(window);
	Model model;
	SdlView view(model, rend);
	while (true) {
		for (const auto& ev : SdlEvents()) {
			if (ev.type == SDL_QUIT) {
				return EXIT_SUCCESS;
			} else if (ev.type == SDL_KEYDOWN) {
				if (ev.key.keysym.sym == SDLK_ESCAPE) {
					requestSdlQuit();
				}
			}
		}
		view.render();
		model.update();
	}
}
