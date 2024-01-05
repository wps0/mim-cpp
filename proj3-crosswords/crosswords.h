#ifndef CROSSWORDS_H
#define CROSSWORDS_H

#include <iostream>
#include <compare>
#include <set>
#include <vector>
#include <optional>

enum orientation_t : bool {
	H, V
};

class RectArea;

using cord_t = size_t;
using pos_t = std::pair<cord_t, cord_t>;
using dim_t = std::pair<cord_t, cord_t>;

constexpr cord_t MAX_COORDINATE = (cord_t) - 1;
constexpr char DEFAULT_CHAR = '?';
constexpr std::string DEFAULT_WORD = "?";
extern char CROSSWORD_BACKGROUND;

extern const RectArea DEFAULT_EMPTY_RECT_AREA;

class Word {
	private:
		pos_t wordStart;
		orientation_t orientation;
		std::string content;

		std::optional<char> at(pos_t pos) const;
		pos_t pos_of_letter(size_t offset) const;
        void construct(size_t x, size_t y);
	public:
        Word(size_t x, size_t y, orientation_t wordOrientation, std::string&& wordContent);
        Word(size_t x, size_t y, orientation_t wordOrientation, std::string& wordContent);
		Word(const Word& word);
		Word(Word&& word);
		Word& operator=(const Word& word);
		Word& operator=(Word&& word);
		inline pos_t get_start_position() const {
			return wordStart;
		}
		pos_t get_end_position() const;
		inline orientation_t get_orientation() const {
			return orientation;
		}
		char at(size_t pos) const;
		inline size_t length() const {
			return content.size();
		}
		std::weak_ordering operator<=>(const Word& word) const;
		bool operator==(const Word& word) const;
		bool operator!=(const Word& word) const;
		RectArea rect_area() const;

		static bool are_letters_the_same(char l1, char l2) {
			return (!isalpha(l1) && !isalpha(l2))
				|| (isalpha(l1) && isalpha(l2) && l1 == l2);
		}

		friend class Crossword;
};

class RectArea {
	private:
		pos_t leftUpper;
		pos_t rightBottom;
		
		bool pointInRect(pos_t point) const;
		bool rectInRect(RectArea rectArea) const;
	public:
		RectArea(pos_t left_top, pos_t right_bottom);
		RectArea(const RectArea& rectArea);
		RectArea(RectArea&& rectArea);
		RectArea& operator=(const RectArea& rectArea);
		RectArea& operator=(RectArea&& rectArea);
		inline pos_t get_left_top() const {
			return leftUpper;
		}
		inline pos_t get_right_bottom() const {
			return rightBottom;
		}
		inline pos_t get_left_bottom() const {
			return {leftUpper.first, rightBottom.second};
		}
		inline pos_t get_right_top() const {
			return {rightBottom.first, leftUpper.second};
		}
		inline void set_left_top(pos_t point) {
			leftUpper = point;
		}
		inline void set_right_bottom(pos_t point) {
			rightBottom = point;
		}
		inline void set_left_bottom(pos_t point) {
			leftUpper.first = point.first;
			rightBottom.second = point.second;
		}
		inline void set_right_top(pos_t point) {
			rightBottom.first = point.first;
			leftUpper.second = point.second;
		}
		const RectArea operator*(const RectArea& rectArea) const;
		RectArea& operator*=(const RectArea& rectArea);
		dim_t size() const;
		inline bool empty() const {
			return
			leftUpper.first > rightBottom.first ||
			leftUpper.second > rightBottom.second;
		}
		void embrace(pos_t point);
};

struct vertical_cmp {
	bool operator()(Word* w1, Word* w2) const;
};

struct horizontal_cmp {
	bool operator()(Word* w1, Word* w2) const;
};

class Crossword {
	private:
		std::set<Word*, horizontal_cmp> h_words;
		std::set<Word*, vertical_cmp> v_words;
		std::vector<Word*> words;
		RectArea area;

		bool does_collide(const Word &w) const;
        bool has_letter_at(pos_t pos, int off_x, int off_y) const {
            pos.first += off_x;
            pos.second += off_y;
            return letter_at(pos).has_value();
        }

    std::optional<char> letter_at(pos_t pos) const;
		std::optional<const Word *> closest_word(const pos_t &pos, orientation_t ori) const;
		void delete_words();

	public:
		Crossword(Word const& first, std::initializer_list<Word> other);
		Crossword(const Crossword& other);
		Crossword(Crossword&& other);
		~Crossword();
		inline dim_t size() const {
			return area.size();
		}
		inline dim_t word_count() const {
			return {h_words.size(), v_words.size()};
		}
		bool insert_word(Word const& w, bool check_collisions = true);
		Crossword& operator=(const Crossword&);
		Crossword& operator=(Crossword&&);
		Crossword operator+(const Crossword& b) const;
		Crossword& operator+=(const Crossword& b);
		friend std::ostream &operator<<(std::ostream &os, const Crossword &crossword);
};

#endif