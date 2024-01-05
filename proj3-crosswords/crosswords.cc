#include <iostream>
#include <algorithm>
#include <compare>
#include <vector>
#include <cctype>
#include "crosswords.h"

#define FIND_CLOSEST_WORD_AND_RETURN(word_set, to) { \
		auto it = (word_set)->upper_bound(&to); \
		if (it != (word_set)->begin()) \
			it--; \
		if (it == (word_set)->end()) \
			return {}; \
		return *it; \
	}

const RectArea DEFAULT_EMPTY_RECT_AREA = RectArea({1, 1}, {0, 0});
char CROSSWORD_BACKGROUND = '.';

// Word implementation:

Word::Word(size_t x, size_t y, orientation_t wordOrientation, std::string&& wordContent) :
        wordStart({x, y}), orientation(wordOrientation), content(wordContent) {
    construct(x, y);
}

Word::Word(size_t x, size_t y, orientation_t wordOrientation, std::string& wordContent) :
        wordStart({x, y}), orientation(wordOrientation), content(wordContent) {
    construct(x, y);
}

Word::Word(const Word& word):
	wordStart(word.wordStart),
	orientation(word.orientation),
	content(word.content) {}

Word::Word(Word&& word):
	wordStart(std::move(word.wordStart)),
	orientation(std::move(word.orientation)),
	content(std::move(word.content)) {}

Word& Word::operator=(const Word& word) {
	wordStart = word.wordStart;
	orientation = word.orientation;
	content = word.content;
	return *this;
}

Word& Word::operator=(Word&& word) {
	wordStart = std::move(word.wordStart);
	orientation = std::move(word.orientation);
	content = std::move(word.content);
	return *this;
}

pos_t Word::get_end_position() const {
	return pos_of_letter(content.size() - 1);
}

char Word::at(size_t pos) const {
	if (pos >= content.size()) return DEFAULT_CHAR;
	return content[pos];
}

std::weak_ordering Word::operator<=>(const Word& word) const {
	if (wordStart.first == word.wordStart.first) {
		if (wordStart.second == word.wordStart.second) {
			if (orientation == word.orientation) {
				return std::weak_ordering::equivalent;
			} else if (orientation == H) {
				return std::weak_ordering::less;
			} else {
				return std::weak_ordering::greater;
			}
		} else {
			return wordStart.second <=> word.wordStart.second;
		}
	} else {
		return wordStart.first <=> word.wordStart.first;
	}
}

bool Word::operator==(const Word& word) const {
	return ((*this <=> word) == std::weak_ordering::equivalent);
}

bool Word::operator!=(const Word& word) const {
	return !(*this == word);
}

RectArea Word::rect_area() const {
	return RectArea(get_start_position(), get_end_position());
}

std::optional<char> Word::at(pos_t pos) const {
	if (wordStart <= pos && pos <= get_end_position()) {
		if (orientation == H && pos.second == wordStart.second) {
			return content[pos.first - wordStart.first];
		} else if (orientation == V && pos.first == wordStart.first) {
			return content[pos.second - wordStart.second];
		}
	}

	return {};
}

pos_t Word::pos_of_letter(size_t offset) const {
	// offset - the position of the letter in the word
	if (orientation == H) {
		return {wordStart.first + offset, wordStart.second};
	} else {
		return {wordStart.first, wordStart.second + offset};
	}
}

void Word::construct(size_t x, size_t y) {
    if (content.empty()) {
        content = DEFAULT_WORD;
    } else {
        size_t max_len = MAX_COORDINATE - (orientation == H ? x : y);
        if (max_len < content.length() - 1)
            content = content.substr(0, max_len + 1);
    }

    std::transform(
            content.begin(),
            content.end(),
            content.begin(),
            ::toupper
    );
}

// RectArea implementation:

bool RectArea::pointInRect(pos_t point) const {
	return
		point.first >= leftUpper.first &&
		point.second >= leftUpper.second &&
		point.first <= rightBottom.first &&
		point.second <= rightBottom.second;
}

bool RectArea::rectInRect(RectArea rectArea) const {
	return
		pointInRect(rectArea.get_left_top()) &&
		pointInRect(rectArea.get_right_top()) &&
		pointInRect(rectArea.get_left_bottom()) &&
		pointInRect(rectArea.get_right_bottom());
}

RectArea::RectArea(pos_t left_top, pos_t right_bottom):
	leftUpper(left_top),
	rightBottom(right_bottom) {}

RectArea::RectArea(const RectArea& rectArea):
	leftUpper(rectArea.leftUpper),
	rightBottom(rectArea.rightBottom) {}

RectArea::RectArea(RectArea&& rectArea):
	leftUpper(std::move(rectArea.leftUpper)),
	rightBottom(std::move(rectArea.rightBottom)) {}

RectArea& RectArea::operator=(const RectArea& rectArea) {
	leftUpper = rectArea.leftUpper;
	rightBottom = rectArea.rightBottom;
	return *this;
}

RectArea& RectArea::operator=(RectArea&& rectArea) {
	leftUpper = std::move(rectArea.leftUpper);
	rightBottom = std::move(rectArea.rightBottom);
	return *this;
}

const RectArea RectArea::operator*(const RectArea& rectArea) const {
	return RectArea(*this) *= rectArea;
}

RectArea& RectArea::operator*=(const RectArea& rectArea) {
	if (empty()) {
		return *this;
	}
	if (rectArea.empty()) {
		set_left_top({1, 1});
		set_right_bottom({0, 0});
		return *this;
	}

	if (rectInRect(rectArea)) {
		set_left_top(rectArea.get_left_top());
		set_right_bottom(rectArea.get_right_bottom());
	}
	else if (rectArea.rectInRect(*this)) {
		set_left_top(get_left_top());
		set_right_bottom(get_right_bottom());
	}
	// rectArea on right-bottom:
	else if (pointInRect(rectArea.get_left_top())) {
		set_left_top(rectArea.get_left_top());
		set_right_bottom(get_right_bottom());
	}
	// rectArea on left-bottom:
	else if (pointInRect(rectArea.get_right_top())) {
		set_right_top(rectArea.get_right_top());
		set_left_bottom(get_left_bottom());
	}
	// rectArea on left-top:
	else if (pointInRect(rectArea.get_right_bottom())) {
		set_left_top(get_left_top());
		set_right_bottom(rectArea.get_right_bottom());
	}
	// rectArea on right-top:
	else if (pointInRect(rectArea.get_left_bottom())) {
		set_right_top(get_right_top());
		set_left_bottom(rectArea.get_left_bottom());
	} else {
		set_left_top({1, 1});
		set_right_bottom({0, 0});
	}

	return *this;
}

dim_t RectArea::size() const {
	if (empty()) return {0, 0};

	size_t width = get_right_top().first - get_left_top().first + 1;
	size_t height = get_left_bottom().second - get_left_top().second + 1;
	return {width, height};
}

void RectArea::embrace(pos_t point) {
	if (pointInRect(point)) return;

	if (empty()) {
		leftUpper = point;
		rightBottom = point;
		return;
	}

	if (point.first < leftUpper.first) {
		leftUpper.first = point.first;
	} else if (point.first > rightBottom.first) {
		rightBottom.first = point.first;
	}

	if (point.second < leftUpper.second) {
		leftUpper.second = point.second;
	} else if (point.second > rightBottom.second) {
		rightBottom.second = point.second;
	}
}

bool vertical_cmp::operator()(Word *w1, Word *w2) const {
	return w1->get_start_position() < w2->get_start_position();
}

bool horizontal_cmp::operator()(Word *w1, Word *w2) const {
	pos_t p1 = w1->get_start_position();
	pos_t p2 = w2->get_start_position();
	std::swap(p1.first, p1.second);
	std::swap(p2.first, p2.second);
	return p1 < p2;
}

Crossword::Crossword(Word const& first, std::initializer_list<Word> other) :
	h_words(),
	v_words(),
	words(),
	area(DEFAULT_EMPTY_RECT_AREA) {
		insert_word(first, false);
		std::for_each(other.begin(), other.end(), [this](Word const& w){
			this->insert_word(w);
		});
}

Crossword::Crossword(const Crossword& other) :
	h_words(),
	v_words(),
	words(),
	area(other.area) {
	for (Word* w : other.words) {
		insert_word(*w, false);
	}
}

Crossword::Crossword(Crossword &&other) :
	h_words(std::move(other.h_words)),
	v_words(std::move(other.v_words)),
	words(std::move(other.words)),
	area(std::move(other.area))  {
    other.words.clear();
    other.h_words.clear();
    other.v_words.clear();
    other.area = DEFAULT_EMPTY_RECT_AREA;
}

Crossword::~Crossword() {
	delete_words();
}

bool Crossword::does_collide(const Word &w) const {
	for (size_t i = 0; i < w.length(); i++) {
		pos_t pos = w.pos_of_letter(i);
		std::optional<char> letter = letter_at(pos);

		if (letter.has_value() && !Word::are_letters_the_same(*letter, w.at(i))) {
			return true;
		} else if (!letter.has_value()) {
			if (w.get_orientation() == H && pos.second > 0) {
				pos.second--;
				if (letter_at(pos).has_value())
					return true;
				pos.second++;
			} else if (w.get_orientation() == V && pos.first > 0) {
				pos.first--;
				if (letter_at(pos).has_value())
					return true;
				pos.first++;
			}

			if (w.get_orientation() == H && pos.second < MAX_COORDINATE) {
				pos.second++;
				if (letter_at(pos).has_value())
					return true;
			} else if (w.get_orientation() == V && pos.first < MAX_COORDINATE) {
				pos.first++;
				if (letter_at(pos).has_value())
					return true;
			}
		}
	}

	pos_t start = w.get_start_position();
	if (w.get_orientation() == H && start.first > 0) {
		start.first--;
		if (letter_at(start).has_value())
			return true;
        start.first++;
	} else if (w.get_orientation() == V && start.second > 0) {
		start.second--;
		if (letter_at(start).has_value())
			return true;
        start.second++;
	}

	pos_t end = w.get_end_position();
	if (w.get_orientation() == H && end.first < MAX_COORDINATE) {
		end.first++;
		if (letter_at(end).has_value())
			return true;
        end.first--;
	} else if (w.get_orientation() == V && end.second < MAX_COORDINATE) {
		end.second++;
		if (letter_at(end).has_value())
			return true;
        end.second--;
	}

    if (w.get_orientation() == V) {
        if (end.second < MAX_COORDINATE) {
            if ((end.first < MAX_COORDINATE && has_letter_at(end, 1, 1))
                || (end.first > 0 && has_letter_at(end, -1, 1)))
                return true;
        }

        if (start.second > 0) {
            if ((start.first > 0 && has_letter_at(start, -1, -1))
                || (start.first < MAX_COORDINATE && has_letter_at(start, 1, -1)))
                return true;
        }
    } else {
        if (end.first < MAX_COORDINATE) {
            if ((end.second > 0 && has_letter_at(end, 1, -1))
                || (end.second < MAX_COORDINATE && has_letter_at(end, 1, 1)))
                return true;
        }

        if (start.first > 0) {
            if ((start.second > 0 && has_letter_at(start, -1, -1))
                || (start.second < MAX_COORDINATE && has_letter_at(start, -1, 1)))
                return true;
        }
    }

	return false;
}

std::optional<char> Crossword::letter_at(pos_t pos) const {
	auto extract_letter = [](pos_t pos, std::optional<const Word *>&& w) {
			if (w.has_value()) {
				std::optional<char> let = w.value()->at(pos);
				if (let.has_value())
					return let;
			}
			return std::optional<char>();
		};

	std::optional<char> l = extract_letter(pos, closest_word(pos, H));
	if (l.has_value())
		return l;
	return extract_letter(pos, closest_word(pos, V));
}

std::optional<const Word*> Crossword::closest_word(const pos_t &pos, orientation_t ori) const {
	if ((ori == H && h_words.empty()) || (ori == V && v_words.empty()))
		return {};

	Word tmp(pos.first, pos.second, ori, "");
	if (ori == H) {
		FIND_CLOSEST_WORD_AND_RETURN(&h_words, tmp);
	} else {
		FIND_CLOSEST_WORD_AND_RETURN(&v_words, tmp);
	}
}

bool Crossword::insert_word(const Word& w, bool check_collisions) {
	if (check_collisions && does_collide(w))
		return false;

	Word *w_ptr = new Word(w);
	words.push_back(w_ptr);
	if (w_ptr->get_orientation() == H)
		h_words.insert(w_ptr);
	else
		v_words.insert(w_ptr);

	area.embrace(w_ptr->get_start_position());
	area.embrace(w_ptr->get_end_position());
	return true;
}

void Crossword::delete_words() {
    for (Word* w_ptr : words)
        delete w_ptr;
    words.clear();
    v_words.clear();
    h_words.clear();
}

Crossword Crossword::operator+(const Crossword& b) const {
	return Crossword(*this) += b;
}

Crossword& Crossword::operator+=(const Crossword& b) {
	for (const Word* w : b.words) {
		insert_word(*w);
	}
	return *this;
}

std::ostream &operator<<(std::ostream &os, const Crossword &crossword) {
	pos_t const& lt = crossword.area.get_left_top();
	pos_t const& rb = crossword.area.get_right_bottom();
	auto print_empty_line = [&lt, &rb, &os]() {
		for (cord_t i = lt.first; i <= rb.first; i++)
			os << CROSSWORD_BACKGROUND << ' ';
		os << CROSSWORD_BACKGROUND << ' ' << CROSSWORD_BACKGROUND << '\n';
	};

	print_empty_line();
	for (cord_t i = lt.second; i <= rb.second; i++) {
		pos_t cur = pos_t(lt.first, i);

		os << CROSSWORD_BACKGROUND << ' ';
		while (cur.first <= rb.first) {
			std::optional<char> letter = crossword.letter_at(cur);
			if (letter.has_value()) {
				os << (isalpha(letter.value()) ? letter.value() : DEFAULT_CHAR);
			} else {
				os << CROSSWORD_BACKGROUND;
			}
			os << ' ';

			cur.first++;
		}
		os << CROSSWORD_BACKGROUND << '\n';
	}
	print_empty_line();

	return os;
}

Crossword& Crossword::operator=(const Crossword& other) {
	delete_words();
	area = other.area;

	for (Word* w : other.words)
		insert_word(*w, false);

	return *this;
}

Crossword& Crossword::operator=(Crossword&& other) {
    delete_words();
    words.swap(other.words);
    h_words.swap(other.h_words);
    v_words.swap(other.v_words);
    area = other.area;
    other.area = DEFAULT_EMPTY_RECT_AREA;
    return *this;
}
