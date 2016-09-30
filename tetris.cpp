#include <SDL.h>

#include <algorithm>
#include <memory>
#include <cassert>
#include <stdexcept>

enum class Cell {
	empty, i, o, t, s, z, j, l
};

template <typename T>
struct Point2 {
	T x, y;

	Point2() : x(), y() {}
	Point2(T x, T y) : x(x), y(y) {}

	template <typename U>
	Point2(const Point2<U>& rhs)
	: x(rhs.x), y(rhs.y)
	{}
};

template <typename T>
Point2<T>&
operator+=(Point2<T>& lhs, Point2<T> rhs)
{
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	return lhs;
}

template <typename T>
Point2<T>
operator+(Point2<T> lhs, Point2<T> rhs)
{
	return lhs += rhs;
}

using Point2i = Point2<int>;
using Point2f = Point2<float>;

struct Playfield {
	enum { width = 10, height = 22 };

	Playfield() { cells_.fill(Cell::empty); }

	Cell operator[](Point2i p) const { return cells_[offset(p)]; }
	Cell& operator[](Point2i p) { return cells_[offset(p)]; }

	void
	collapseFullLines()
	{
		for (int y = height - 1; y;) {
			if (isLineFull(y)) {
				scrollDownTill(y);
			} else {
				--y;
			}
		}
	}

private:
	using Cells = std::array<Cell, width * height>;

	bool
	isLineFull(int y)
	{
		for (int x = 0; x != width; ++x) {
			if ((*this)[Point2i{x, y}] == Cell::empty) return false;
		}
		return true;
	}

	void
	scrollDownTill(int yy)
	{
		for (int y = yy - 1; y; --y) {
			for (int x = 0; x != width; ++x) {
				(*this)[Point2i{x, y + 1}] = (*this)[Point2i{x, y}];
				// TODO: upper line?
			}
		}
	}

	static int
	offset(Point2i p)
	{
		assert(p.x >= 0 && p.x < width);
		assert(p.y >= 0 && p.y < height);
		return p.y * width + p.x;
	}

	Cells cells_;
};

using TetroShape = std::array<Point2f, 4>;

struct TetroBreed {
	virtual ~TetroBreed() {}

	virtual Cell getColor() const = 0;
	virtual TetroShape getShape() const = 0;
};

class ITetroBreed : public TetroBreed {
	Cell getColor() const override { return Cell::i; }
	TetroShape getShape() const override { return {{{-1.5, 0.5}, {-0.5, 0.5}, {0.5, 0.5}, {1.5, 0.5}}}; }
};

template <class Breed>
const TetroBreed*
instance()
{
	static const Breed b;
	return &b;
}

struct Tetromino {
	Tetromino() { respawn(); }

	Cell getColor() const { return breed_->getColor(); }

	TetroShape
	getShape()
	const
	{
		auto result = breed_->getShape();
		std::transform(result.begin(), result.end(), result.begin(), [this](Point2f p){ return p + position_; });
		return result;
	}

	void move(Point2f d) { position_ += d; }

	void
	respawn()
	{
		static const TetroBreed* breeds[] = {
			  instance<ITetroBreed>()
		};
		position_ = {4.5, 0};
		breed_ = breeds[rand() % (sizeof(breeds) / sizeof(breeds[0]))];
	}

private:
	Point2f position_;
	const TetroBreed* breed_{};
};

struct Model {
	void update() { stepDown(); }
	void moveLeft() { checkAndMove({-1, 0}); }
	void moveRight() { checkAndMove({1, 0}); }
	void drop() { while (stepDown()); }

	const Playfield& getPlayfield() const { return pf_; }
	const Tetromino& getTetromino() const { return tm_; }

private:
	bool
	stepDown()
	{
		const bool result = checkAndMove({0, 1});
		if (!result) freezeCurrentPiece();
		return result;
	}

	bool
	checkAndMove(Point2f d)
	{
		bool result = mayMove(tm_.getShape(), d);
		if (result) tm_.move(d);
		return result;
	}

	bool
	mayMove(const TetroShape& ts, Point2i dir)
	const
	{
		return std::all_of(ts.begin(), ts.end(), [this, dir](Point2i p){ return isFree(p + dir); });
	}

	bool
	isFree(Point2i p)
	const
	{
		if (p.x < 0 || p.x >= pf_.width) return false;
		if (p.y < 0 || p.y >= pf_.height) return false;
		return pf_[p] == Cell::empty;
	}

	void
	freezeCurrentPiece()
	{
		freeze(tm_.getShape(), tm_.getColor());
		tm_.respawn();
		pf_.collapseFullLines();
	}

	void
	freeze(const TetroShape& ts, Cell color)
	{
		std::for_each(ts.begin(), ts.end(), [this, color](Point2i p){ pf_[p] = color; });
	}

	Playfield pf_;
	Tetromino tm_;
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
	void drawLine(Point2i a, Point2i b) { SDL_RenderDrawLine(rend_.get(), a.x, a.y, b.x, b.y); }
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
		render(model_.getTetromino());
		rend_.present();
	}

private:
	static const int cellsize = 20;

	void
	render(const Playfield& pf)
	{
		rend_.setDrawColor(SdlColor::gray());
		for (int y = 0; y != pf.height + 1; ++y) {
			rend_.drawLine(Point2i{0, y * cellsize}, Point2i{pf.width * cellsize, y * cellsize});
		}
		for (int x = 0; x != pf.width + 1; ++x) {
			rend_.drawLine(Point2i{x * cellsize, 0}, Point2i{x * cellsize, pf.height * cellsize});
		}
		for (int y = 0; y != pf.height; ++y) {
			for (int x = 0; x != pf.width; ++x) {
				rend_.setDrawColor(getColor(pf[Point2i{x, y}]));
				rend_.fillRect(SDL_Rect{x * cellsize + 1, y * cellsize + 1, cellsize - 1, cellsize - 1});
			}
		}
	}

	void
	render(const Tetromino& tm)
	{
		rend_.setDrawColor(getColor(tm.getColor()));
		render(tm.getShape());
	}

	void
	render(const TetroShape& ts)
	{
		std::for_each(ts.begin(), ts.end(), [this](Point2i p){ render(p); });
	}

	void
	render(const Point2i p)
	{
		rend_.fillRect(SDL_Rect{p.x * cellsize + 1, p.y * cellsize + 1, cellsize - 1, cellsize - 1});
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

struct SdlControl {
	SdlControl(Model& model) : model_(model) {}

	void update() { for (const auto& ev : SdlEvents()) process(ev); }

	bool shouldQuit() const { return quit_; }

private:
	void
	process(const SDL_Event& ev)
	{
		if (ev.type == SDL_QUIT) {
			quit_ = true;
		} else if (ev.type == SDL_KEYDOWN) {
			process(ev.key);
		}
	}

	void
	process(const SDL_KeyboardEvent& kev)
	{
		switch (kev.keysym.sym) {
		case SDLK_ESCAPE: requestSdlQuit(); break;
		case SDLK_LEFT: model_.moveLeft(); break;
		case SDLK_RIGHT: model_.moveRight(); break;
		case SDLK_DOWN: model_.drop(); break;
		}
	}

	Model& model_;
	bool quit_{};
};

int
main()
{
	Sdl sdl;
	auto window = sdl.createWindow();
	SdlRenderer rend(window);
	Model model;
	SdlControl control(model);
	SdlView view(model, rend);
	for (int i = 0; !control.shouldQuit(); ++i) {
		control.update();
		if (!(i & 15)) model.update();
		view.render();
	}
}
