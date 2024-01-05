#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <queue>
#include <unordered_map>

namespace {

  // ----- Type aliases ----- //

  using minute_t  = int_fast16_t;
  using hour_t    = minute_t;
  using day_t     = size_t;                     // day count is bounded by the number of lines
  using time_t    = std::pair<hour_t, minute_t>;
  using date_t    = std::pair<day_t, time_t>;

  namespace parser {
    const minute_t MINUTES_IN_HOUR = 60;
    const minute_t MIN_PAID_MINUTES = 10;
    const minute_t MAX_PAID_MINUTES = 12 * MINUTES_IN_HOUR;

    const std::string HOUR_PATTERN = "(0[8-9]|[8-9]|1[0-9]|20(?=\\.00))\\.([0-5][0-9])";
    const std::string NUMBER_PLATE_PATTERN = "([A-Z][0-9A-Z]{2,10})";
    const std::string QUERY_PATTERN = std::string("^\\s*?")
                                        .append(NUMBER_PLATE_PATTERN)
                                        .append("\\s+?")
                                        .append(HOUR_PATTERN)
                                        .append("\\s*?$");
    const std::string UPDATE_PATTERN = std::string("^\\s*?")
                                        .append(NUMBER_PLATE_PATTERN)
                                        .append("\\s+?")
                                        .append(HOUR_PATTERN)
                                        .append("\\s+?")
                                        .append(HOUR_PATTERN)
                                        .append("\\s*?$");
    const std::regex QUERY_REGEX(QUERY_PATTERN);
    const std::regex UPDATE_REGEX(UPDATE_PATTERN);

    minute_t to_minutes(time_t t) {
      return t.first * MINUTES_IN_HOUR + t.second;
    }

    minute_t paid_minutes(time_t from, time_t to) {
      minute_t start = to_minutes(from);
      minute_t end = to_minutes(to);

      minute_t paid_minutes = end - start;
      if (end < start) {
        // ----[***end----start*]--->
        // Paid time is marked with *, MAX_PARKING_DURATION is the duration of
        //  [...] segment.
        paid_minutes += MAX_PAID_MINUTES;
      }
      return paid_minutes;
    }

    bool validate_timespan(time_t from, time_t to) {
      minute_t pm = paid_minutes(from, to);
      if (pm < MIN_PAID_MINUTES || pm >= MAX_PAID_MINUTES) {
        return false;
      }
      return true;
    }
  }

  namespace logic {
    // Parking entry {ending date, plate number}.
    using entry_t = std::pair<date_t, std::string>;

    date_t cur_date;
    // Active entries sorted lexicographically.
    std::priority_queue<entry_t, std::vector<entry_t>, std::greater<entry_t>> active_entries;
    // Each active plate has an entry with the number of its active tickets.
    std::unordered_map<std::string, size_t> plate_count;
    
    void init() {
      cur_date = {0, {0, 0}};
    }

    void remove_entry() {
      auto entry = active_entries.top();
      auto plate_entry = plate_count.find(entry.second);
      plate_entry->second--;

      if (plate_entry->second == 0) {
        plate_count.erase(plate_entry);
      }

      active_entries.pop();
    }

    void update_cur_date(time_t new_time) {
      // Update cur_date value.
      if (new_time < cur_date.second) {
        cur_date.first++;
      }
      cur_date.second = new_time;

      // Remove inactive entries.
      while (!active_entries.empty() && active_entries.top().first < cur_date) {
        remove_entry();
      }
    }

    // Updates current date and answers the query.
    bool is_paid(const std::string& plate_number, time_t time) {
      update_cur_date(time);

      return plate_count.find(plate_number) != plate_count.end();
    }

    // Updates current date and adds the entry.
    void add_entry(const std::string& plate_number, time_t from, time_t to) {
      update_cur_date(from);
      
      auto plate_entry = plate_count.find(plate_number);
      if (plate_entry == plate_count.end()) {
        plate_entry = plate_count.insert({plate_number, 0}).first;
      }
      plate_entry->second++;

      size_t day = cur_date.first + (to < from ? 1 : 0);
      active_entries.push({{day, to}, plate_number});
    }
  }

  // ----- Printing functions ----- //

  void confirm_entry(size_t line) {
    std::cout << "OK " << line << std::endl;
  }

  void confirm_paid(size_t line) {
    std::cout << "YES " << line << std::endl;
  }

  void confirm_not_paid(size_t line) {
    std::cout << "NO " << line << std::endl;
  }

  void confirm_error(size_t line) {
    std::cerr << "ERROR " << line << std::endl;
  }

  // ----- Main function ----- //

  void run() {
    size_t line_number = 1;
    logic::init();
    std::string line;

    while (std::getline(std::cin, line)) {

      std::smatch results;
      if (std::regex_match(line, results, parser::QUERY_REGEX)) {
        std::string plate_number = results[1];
        time_t time = {std::stoi(results[2]), std::stoi(results[3])};

        if (logic::is_paid(plate_number, time)) {
          confirm_paid(line_number);
        } else {
          confirm_not_paid(line_number);
        }

      } else if (std::regex_match(line, results, parser::UPDATE_REGEX)) {
        std::string plate_number = results[1];
        time_t time_from = {std::stoi(results[2]), std::stoi(results[3])};
        time_t time_to = {std::stoi(results[4]), std::stoi(results[5])};

        if (!parser::validate_timespan(time_from, time_to)) {
          confirm_error(line_number);
        } else {
          logic::add_entry(plate_number, time_from, time_to);
          confirm_entry(line_number);
        }
      } else {
        confirm_error(line_number);
      }

      line_number++;
    }
  }
}

int main() {
  run();
}

