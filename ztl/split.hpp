#include <algorithm>
#include <functional>
#include <iterator>
#include <utility>

namespace ztl {

template<class It, class DelimPred>
class basic_splitter {
public:
	using iterator = It;
	using value_type = typename std::iterator_traits<iterator>::value_type;
	using range_type = std::pair<iterator, iterator>; 

	basic_splitter(iterator first, iterator last, DelimPred isdelim)
		:_first(first), _last(last), _prev(first), _isdelim(isdelim)
	{}

	range_type next() {
		if (_finished)
			return {_last, _last};
		for (; _first != _last; ++_first) {
			if (_isdelim(*_first)) {
				iterator _oldprev = _prev;
				_prev = std::next(_first);
				return {_oldprev, _first++};
			}
		}
		_finished = true;
		return {_prev, _last};
	}

	template<class Func>
	decltype(auto) yield(Func f) {
#if __cplusplus >= 201703L
		return std::apply(f, next());
#else
		auto args = next();
		return f(args.first, args.second);
#endif
	}

	template<class Func>
	void yield_all(Func f) {
		while (*this) {
			yield(f);
		}
	}


	bool finished() const noexcept {
		return _finished;
	}
	operator bool () const noexcept {
		return !finished();
	}

private:
	iterator _first, _last, _prev;
	bool _finished = false;
	DelimPred _isdelim;
};


template<class It, class DelimPred>
static inline basic_splitter<It, DelimPred> splitter_if(It first, It last, DelimPred isdelim)
{
	return basic_splitter<It, DelimPred>(first, last, isdelim);
}

template<class It, class Jt>
static inline auto splitter_in(It first, It last, Jt dfirst, Jt dlast)
{
	auto isdelim = [dfirst, dlast](auto ch) {
		return std::find(dfirst, dlast, ch) != dlast;
	};
	return splitter_if(first, last, isdelim);
}

template<class It, class DelimCont>
static inline auto splitter_in(It first, It last, const DelimCont& delims)
{
	return splitter_in(first, last, std::begin(delims), std::end(delims));
}

template<class Cont>
struct back_emplacer {
	back_emplacer(Cont& cont)
		:_cont(cont)
	{ }

	template<class... Args>
	decltype(auto) operator() (Args&&... args) const
	{
		return _cont.emplace_back(std::forward<Args>(args)...);
	}

	Cont& _cont;
};

}
