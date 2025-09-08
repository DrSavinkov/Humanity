#include "myrand.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <ostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
using namespace std;
const bool DEBUG = false;
template <typename C, typename T> vector<T> shuffle(const C &container) {
  vector<T> r(container.begin(), container.end());
  random_device rd;
  mt19937 generator(rd());
  shuffle(r.begin(), r.end(), generator);
  return r;
}
template <typename T> vector<T> shuffle(const unordered_set<T> &container) {
  return shuffle<unordered_set<T>, T>(container);
}
unordered_set<string> intersect(const unordered_set<string> &a,
                                const unordered_set<string> &b) {
  unordered_set<string> r;
  for (const auto &i : a)
    if (b.count(i))
      r.insert(i);
  return r;
}
class Tick {
  static uint64_t _tick;

public:
  static uint64_t get() { return _tick; }
  static void tick() { _tick++; }
};
bool is_natural(const std::string &s) {
  try {
    int value = stoi(s);
    return true;
  } catch (invalid_argument ia) {
    if (DEBUG)
      std::cerr << "\"" << s << "\" is not a number.\n";
  } catch (out_of_range oor) {
    if (DEBUG)
      std::cerr << "\"" << s << "\" is too large.\n";
  }
  return false;
}
uint64_t Tick::_tick = 0;

bool compare(const int &a, const string &op, const int &b) {
  if (op == "=")
    return a == b;
  else if (op == ">")
    return a > b;
  else if (op == "<")
    return a < b;
  else if (op == "<>")
    return a != b;
  else if (op == ">=")
    return a >= b;
  else if (op == "<=")
    return a <= b;
  return false;
}

vector<string> split(string s, const string &key, const bool trim = true) {
  vector<string> r;
  size_t pos;
  while ((pos = s.find(key)) != s.npos) {
    r.push_back(s.substr(0, pos));
    s = s.substr(pos + key.length());
  }
  auto trim_proc = [](string &s) -> void {
    while (s.length()) {
      if (s[0] == ' ')
        s = s.substr(1);
      else if (s.back() == ' ')
        s = s.substr(0, s.length() - 1);
      else
        break;
    }
  };
  r.push_back(s);
  for (uint64_t i = 0; i < r.size(); i++)
    trim_proc(r[i]);
  for (uint64_t i = r.size() - 1; i < r.size(); i--) {
    if (r[i].length() == 0)
      r.erase(r.begin() + i);
  }
  return r;
}

struct Human;
struct Vacancy;
struct Job;
struct Location;

class Action {
  string name;
  uint64_t price, time_to_execute, bonus_money; // цена в рублях, время в часах
  unordered_set<string> tags;
  unordered_map<string, uint64_t> rules, items, removable_items;

public:
  const auto &get_name() const { return name; }
  const auto &get_price() const { return price; }
  const auto &get_time_to_execute() const { return time_to_execute; }
  const auto &get_bonus_money() const { return bonus_money; }
  const auto &get_tags() const { return tags; }
  const auto &get_rules() const { return rules; }
  const auto &get_items() const { return items; }
  const auto &get_removable_items() const { return removable_items; }
  bool operator<(const Action &a) const { return name < a.name; }
  Action(const string &name, const uint64_t &price,
         const uint64_t &time_to_execute, const unordered_set<string> &tags,
         const unordered_map<string, uint64_t> &rules,
         const unordered_map<string, uint64_t> &items,
         const unordered_map<string, uint64_t> &removable_items,
         const uint64_t bonus_money) {
    this->name = name;
    this->price = price;
    this->time_to_execute = time_to_execute;
    this->tags = tags;
    this->rules = rules;
    this->items = items;
    this->removable_items = removable_items;
    this->bonus_money = bonus_money;
  }
  bool executable(const Human *person) const;
  void apply(Human *person) const;
};
class LocalTarget {
  string name;
  unordered_set<string> tags;
  unordered_set<Action *> actions_possible, actions_executed;

public:
  void mark_as_executed(Action *action) {
    actions_possible.erase(action);
    actions_executed.insert(action);
  }
  bool is_executed_full() const {
    unordered_set<string> buf = tags;
    for (const auto &i : actions_executed)
      for (const auto &j : i->get_tags())
        buf.erase(j);
    return buf.size() == 0;
  }
  const auto &get_name() const { return name; }
  const auto &get_tags() const { return tags; }
  const auto &get_actions_possible() const { return actions_possible; }
  const auto &get_actions_executed() const { return actions_executed; }
  LocalTarget(const string &name, const unordered_set<string> &tags,
              const vector<Action *> &all_possible_actions) {
    this->name = name;
    this->tags = tags;
    for (const auto &i : all_possible_actions)
      if (intersect(this->tags, i->get_tags()).size())
        actions_possible.insert(i);
  }
  bool executable(const Human *person) const;
  bool operator<(const LocalTarget &lt) const { return name < lt.name; }
  Action *choose_action(Human *person) const {
    map<uint64_t, vector<Action *>> rating;
    unordered_set<string> left_tags = tags;
    for (const auto &i : actions_executed)
      for (const auto &j : i->get_tags())
        left_tags.erase(j);
    for (const auto &i : actions_possible)
      if (i->executable(person)) {
        uint64_t rate = 0;
        for (const auto &j : i->get_tags())
          rate += left_tags.count(j);
        rating[rate].push_back(i);
      }
    if (rating.size()) {
      // берём действие, закрывающее максимум незакрытых тегов
      vector<Action *> take_from = rating.rbegin()->second;
      return take_from[my::Random::next() % take_from.size()];
    }
    return nullptr;
  }
};
class GlobalTarget {
  string name;
  unordered_set<string> tags;
  double power;
  unordered_set<LocalTarget *> targets_possible, targets_executed;

public:
  void mark_as_executed(LocalTarget *ltarget) {
    targets_possible.erase(ltarget);
    targets_executed.insert(ltarget);
  }
  bool is_executed_full() const {
    unordered_set<string> buf = tags;
    for (const auto &i : targets_executed)
      for (const auto &j : i->get_tags())
        buf.erase(j);
    return buf.size() == 0;
  }
  GlobalTarget(const GlobalTarget &gt) {
    name = gt.name;
    tags = gt.tags;
    power = gt.power;
    for (const auto &i : gt.targets_possible)
      targets_possible.insert(new LocalTarget(*i));
    for (const auto &i : gt.targets_executed)
      targets_executed.insert(new LocalTarget(*i));
  }
  GlobalTarget &operator=(const GlobalTarget &gt) {
    name = gt.name;
    tags = gt.tags;
    power = gt.power;
    for (const auto &i : gt.targets_possible)
      targets_possible.insert(new LocalTarget(*i));
    for (const auto &i : gt.targets_executed)
      targets_executed.insert(new LocalTarget(*i));
    return *this;
  }
  const auto &get_name() const { return name; }
  const auto &get_tags() const { return tags; }
  const auto &get_power() const { return power; }
  const auto &get_targets_possible() const { return targets_possible; }
  const auto &get_targets_executed() const { return targets_executed; }
  bool executable(const Human *person) const;
  bool operator<(const GlobalTarget &gt) const { return name < gt.name; }
  GlobalTarget(const string &name, const unordered_set<string> &tags,
               const double &power,
               const vector<LocalTarget *> &all_possible_targets) {
    this->name = name;
    this->tags = tags;
    this->power = power;
    for (const auto &i : all_possible_targets) {
      if (intersect(this->tags, i->get_tags()).size())
        targets_possible.insert(i);
    }
  }
  LocalTarget *choose_target(Human *person) const {
    map<uint64_t, vector<LocalTarget *>> rating;
    unordered_set<string> left_tags = tags;
    for (const auto &i : targets_executed)
      for (const auto &j : i->get_tags())
        left_tags.erase(j);
    // для глобальной цели надо выбрать локальные цели, закрывающие максимум
    // из не закрытых тегов, которые мы можем выполнить полностью или
    // довыполнить...
    for (const auto &i : targets_possible)
      if (i->executable(person)) {
        uint64_t rate = 0;
        for (const auto &j : i->get_tags())
          rate += left_tags.count(j);
        rating[rate].push_back(i);
      }
    if (false)
      if (rating.size() == 0 && targets_possible.size()) {
        // ни одна из локальных целей не может быть достигнута/не имеет нужных
        // тегов
        // выбираем случайную
        for (const auto &i : targets_possible)
          rating[0].push_back(i);
      }
    if (rating.size()) {
      // берём локальную цель, закрывающую максимум незакрытых тегов
      vector<LocalTarget *> take_from = rating.rbegin()->second;
      return take_from[my::Random::next() % take_from.size()];
    }
    return nullptr;
  }
};

ostream &operator<<(ostream &os, const Action &a) {
  os << "Action \"" << a.get_name() << "\":\n";
  os << "  Price: " << a.get_price() << endl;
  os << "  Time to execute: " << a.get_time_to_execute() << endl;
  if (a.get_rules().size()) {
    os << "  Rules:\n";
    for (const auto &i : a.get_rules())
      os << "    " << i.first << " [" << i.second << "]" << endl;
  }
  if (a.get_tags().size()) {
    os << "  Tags:\n";
    for (const auto &i : a.get_tags())
      os << "    " << i << endl;
  }
  if (a.get_removable_items().size()) {
    os << "  Removable items:\n";
    for (const auto &i : a.get_removable_items())
      os << "    " << i.first << " [" << i.second << "]" << endl;
  }
  if (a.get_items().size()) {
    os << "  items:\n";
    for (const auto &i : a.get_items())
      os << "    " << i.first << " [" << i.second << "]" << endl;
  }
  return os;
}
vector<vector<string>> load_sequences_from_file(const string filename) {
  ifstream fin(filename);
  vector<vector<string>> r;
  string line;
  while (getline(fin, line)) {
    // пропускаем строки-комментарии, начинающиеся с #
    if (line.length() && line[0] == '#')
      continue;
    stringstream sline(line);
    vector<string> words = split(line, " ");
    r.push_back(words);
  }
  return r;
}

// Действия (конкретные шаги)
const vector<Action *> ACTIONS = [](const string filename) -> vector<Action *> {
  vector<Action *> r;
  for (const auto &words : load_sequences_from_file(filename)) {
    if (words.size() > 3) {
      string name = words[0];
      uint64_t price = stoi(words[1]);
      uint64_t tte = stoi(words[2]);
      uint64_t bonus = 0;
      unordered_set<string> tags;
      unordered_map<string, uint64_t> rules, items, removable;
      for (uint64_t i = 3; i < words.size(); i++) {
        if (words[i].find("$") != words[i].npos) {
          if (words[i].length() > 1 && words[i][1] == '-') {
            auto key = words[i].substr(2);
            if (removable.count(key))
              removable[key]++;
            else
              removable[key] = 1;
          } else {
            auto key = words[i].substr(1);
            if (rules.count(key))
              rules[key]++;
            else
              rules[key] = 1;
          }
        } else if (words[i].find("@") != words[i].npos) {
          auto key = words[i].substr(1);
          if (items.count(key))
            items[key]++;
          else
            items[key] = 1;
        } else if (words[i].find("+") != words[i].npos) {
          bonus = std::stoul(words[i].substr(1));
        } else
          tags.insert(words[i]);
      }
      r.push_back(
          new Action(name, price, tte, tags, rules, items, removable, bonus));
    } else {
      throw(invalid_argument("Check actions file."));
    }
  }
  return r;
}("actions.ini");

// Локальные цели (краткосрочные)
const vector<LocalTarget *> LOCAL_TARGETS =
    [](const string filename) -> vector<LocalTarget *> {
  vector<LocalTarget *> r;
  for (const auto &words : load_sequences_from_file(filename)) {
    if (words.size() > 1) {
      r.push_back(
          new LocalTarget(words[0], {++words.begin(), words.end()}, ACTIONS));
    } else {
      throw(invalid_argument("Check local targets file."));
    }
  }
  return r;
}("local.ini");

// Глобальные цели (долгосрочные)
const vector<GlobalTarget *> GLOBAL_TARGETS =
    [](const string filename) -> vector<GlobalTarget *> {
  vector<GlobalTarget *> r;
  for (const auto &words : load_sequences_from_file(filename)) {
    if (words.size() > 1) {
      try {
        r.push_back(new GlobalTarget(words[0], {words.begin() + 2, words.end()},
                                     stof(words[1]), LOCAL_TARGETS));
      } catch (...) {
        // ловим ошибки, если в файле с глобальными целями не указана мощность
        r.push_back(new GlobalTarget(words[0], {words.begin() + 1, words.end()},
                                     1.0, LOCAL_TARGETS));
      }
    } else {
      throw(invalid_argument("Check global targets file."));
    }
  }
  return r;
}("global.ini");

const unordered_map<string, GlobalTarget *> GLOBAL_NAME = []() {
  unordered_map<string, GlobalTarget *> r;
  for (const auto &i : GLOBAL_TARGETS) {
    if (r.count(i->get_name()))
      throw(invalid_argument("Global target name duplication!"));
    r[i->get_name()] = i;
  }
  return r;
}();
const unordered_map<string, LocalTarget *> LOCAL_NAME = []() {
  unordered_map<string, LocalTarget *> r;
  for (const auto &i : LOCAL_TARGETS) {
    if (r.count(i->get_name()))
      throw(invalid_argument("Local target name duplication!"));
    r[i->get_name()] = i;
  }
  return r;
}();
const unordered_map<string, Action *> ACTION_NAME = []() {
  unordered_map<string, Action *> r;
  for (const auto &i : ACTIONS) {
    if (r.count(i->get_name()))
      throw(invalid_argument("Action name duplication!"));
    r[i->get_name()] = i;
  }
  return r;
}();

struct Path {
  Location *from, *to;
  uint64_t price, time;
  Path(Location *from, Location *to, const uint64_t &price,
       const uint64_t &time) {
    this->from = from;
    this->to = to;
    this->price = price;
    this->time = time;
  }
};
struct Splash {
  string name;
  unordered_set<string> tags;
  uint64_t appear_time, life_length;
  Splash(const string &name, const unordered_set<string> &tags,
         const uint64_t &life_length) {
    this->name = name;
    this->tags = tags;
    this->appear_time = Tick::get();
    this->life_length = life_length;
  }
};
struct Human {
  double age = []() -> double {
    random_device rd;
    mt19937 gen(rd());
    normal_distribution<double> age_distribution(25.0, 10.0);
    return max(0., min(80., age_distribution(gen)));
  }();
  bool dead = false;
  uint64_t busy_hours = 0;
  int64_t money = 7000;
  Vacancy *job = nullptr;
  uint64_t job_time = 720;
  Location *home_location = nullptr;
  // с кем и как давно связан
  unordered_map<Human *, double> parents, family, children, friends;
  vector<Splash> splashes;
  unordered_set<GlobalTarget *> global_targets, completed_global_targets;
  unordered_map<string, int64_t> items;
  Human(const unordered_set<Human *> &parent, Location *home_location) {
    for (const auto &i : parent)
      this->parents[i] = 0.;
    this->home_location = home_location;
    const int ngt = min(GLOBAL_TARGETS.size(), 2 + my::Random::next() % 2);
    while (global_targets.size() < ngt) {
      auto ptr = GLOBAL_TARGETS.begin();
      uint64_t offset = my::Random::next() % GLOBAL_TARGETS.size();
      while (offset--)
        ptr++;
      // закладываем частные копии глобальных целей агенту
      bool isnew = true;
      // проверяем, не взяли ли мы вторую такую же цель
      for (const auto &i : global_targets)
        if (i->get_name() == (*ptr)->get_name())
          isnew = false;
      if (isnew) {
        global_targets.insert(new GlobalTarget(**ptr));
        cerr << "[DEBUG] unit " << this;
        cerr << " got GT " << (*ptr)->get_name() << ":";
        for (const auto &i : (*ptr)->get_tags())
          cerr << " " << i;
        cerr << endl;
      }
    }
  }
  void iterate_hour();
};
struct Vacancy {
  Job *parent;
  unordered_set<string> required_tags; // education, skills, etc
  int payment;                         // monthly payment
};
struct Job {
  unordered_map<Vacancy *, uint64_t> vacant_places;
  Location *home_location = nullptr;
};
struct Location {
  unordered_set<Job *> jobs;
  unordered_set<Human *> humans;
  unordered_set<Path *> paths;
};
void Human::iterate_hour() {
  if (money <= 0) {
    // агент замечает, что денег не хватает, появляется мысль о поиске новой
    // работы
    splashes.push_back(
        Splash("need_money", {"деньги", "благополучие", "карьера"}, 24));
  }
  for (auto &i : parents)
    i.second += 1. / (24 * 365);
  for (auto &i : family)
    i.second += 1. / (24 * 365);
  for (auto &i : children)
    i.second += 1. / (24 * 365);
  for (auto &i : friends)
    i.second += 1. / (24 * 365);
  if (job == nullptr) {
    // в любой момент можно пойти искать работу
    job_time = 721;
  } else {
    job_time++;
  }
  // удаляем просроченные всплески
  vector<uint64_t> erase;
  for (uint64_t i = 0; i < splashes.size(); i++)
    if (Tick::get() - splashes[i].appear_time > splashes[i].life_length)
      erase.push_back(i);
  for (const auto &i : erase) {
    swap(splashes[i], splashes.back());
    splashes.pop_back();
  }
  erase.clear();
  if (age > 80.) {
    if (!dead) {
      // тут прописать перераспределение остатков денег от умершего к
      // родственникам
      if (money > 0) {
        // долги умерший оставляет государству
        unordered_set<Human *> candidates;
        for (const auto &i : family)
          if (!i.first->dead)
            candidates.insert(i.first);
        for (const auto &i : children)
          if (!i.first->dead)
            candidates.insert(i.first);
        for (const auto &i : parents)
          if (!i.first->dead)
            candidates.insert(i.first);
        if (candidates.size()) {
          for (auto &i : candidates)
            i->money += money / candidates.size();
        }
      }
      // обнуляем бюджет умершего
      money = 0;
    }
    dead = true;
  }
  if (dead)
    return;
  age += 1. / (24 * 365); // стареем
  if (Tick::get() % 24 == 0) {
    money -= 500; // дневные траты
    if (job != nullptr && Tick::get() % (30 * 24) == 0)
      money += job->payment;
  }
  // если ушли в минус по бюджету, то пробуем перераспределить деньги в
  // семье, от детей, от родителей (порядок обсуждаем)
  if (money < 0) {
    for (auto &i : family) {
      if (money < 0 && i.first->money > 0) {
        const int value = min(-money, i.first->money);
        money += value;
        i.first->money -= value;
      }
    }
    for (auto &i : children) {
      if (money < 0 && i.first->money > 0) {
        const int value = min(-money, i.first->money);
        money += value;
        i.first->money -= value;
      }
    }
    for (auto &i : parents) {
      if (money < 0 && i.first->money > 0) {
        const int value = min(-money, i.first->money);
        money += value;
        i.first->money -= value;
      }
    }
  }
  if (busy_hours) {
    busy_hours--;
  } else {
    // тут активные действия
    // вычисляем рейтинги глобальных действий согласно всплескам
    if (global_targets.size()) {
      map<double, vector<GlobalTarget *>> rating;
      if (splashes.size()) {
        for (const auto &i : global_targets) {
          uint64_t counter = 0;
          for (const auto &j : splashes) {
            // подсчитываем, сколько всплесков ассоциировано с целью через общие
            // теги
            counter += (intersect(i->get_tags(), j.tags).size() > 0);
          }
          rating[(i->get_power() * counter) / splashes.size()].push_back(i);
        }
      } else {
        for (const auto &i : global_targets) {
          rating[i->executable(this) * i->get_power()].push_back(i);
        }
      }
      if (false)
        if (rating.size() == 0) {
          // ни одна из глобальных целей не может быть достигнута полностью
          // тогда попытаемся хоть частично достичь случайную
          for (const auto &i : global_targets) {
            rating[0].push_back(i);
          }
        }
      auto first_line = rating.rbegin()->second;
      GlobalTarget *selected_global_target =
          first_line[my::Random::next() % first_line.size()];
      auto selected_local_target = selected_global_target->choose_target(this);
      if (selected_local_target != nullptr) {
        auto selected_action = selected_local_target->choose_action(this);
        if (selected_action != nullptr) {
          // if (DEBUG)
          if (true) {
            cerr << "Unit " << this << " chosen a \""
                 << selected_global_target->get_name();
            cerr << "\" global target, \"" << selected_local_target->get_name()
                 << "\", ";
            cerr << "\"" << selected_action->get_name() << "\" action.\n";
          }
          selected_action->apply(this);
          selected_local_target->mark_as_executed(selected_action);
          if (selected_local_target->is_executed_full()) {
            cerr << "Local target\" " << selected_local_target->get_name()
                 << "\" executed.\n";
            selected_global_target->mark_as_executed(selected_local_target);
            if (selected_global_target->is_executed_full()) {
              cerr << "Global target\" " << selected_global_target->get_name()
                   << "\" executed.\n";
              completed_global_targets.insert(selected_global_target);
              global_targets.erase(selected_global_target);
            }
          }
        }
      }
    }
  }
}
void Action::apply(Human *person) const {
  // не проверяем, мог ли агент сделать действие, проверяйте до применения
  person->money -= price;
  for (const auto &i : items)
    person->items[i.first] += i.second;
  for (const auto &i : removable_items) {
    person->items[i.first] -= i.second;
    // если у владельца больше нет экземпляров вещи, удаляем её из инвентаря
    if (person->items[i.first] == 0)
      person->items.erase(i.first);
    // если вещей стало меньше 0, это ошибка!
    if (person->items[i.first] < 0)
      throw(out_of_range("Unit have negative amount of items!"));
  }
  person->money += bonus_money;
  // частный случай: специальное событие: поиск новой работы
  if (name == "найти_работу") {
    vector<Vacancy *> possible_jobs_in_home_location;
    for (const auto &i : person->home_location->jobs) {
      for (const auto &j : i->vacant_places) {
        if (j.second > 0 && (person->job == nullptr ||
                             j.first->payment > person->job->payment)) {
          bool suitable = true;
          for (const auto &k : j.first->required_tags) {
            if (!person->items.count(k))
              suitable = false;
          }
          if (suitable) {
            possible_jobs_in_home_location.push_back(j.first);
          }
        }
      }
    }
    if (possible_jobs_in_home_location.size()) {
      auto chosen =
          possible_jobs_in_home_location[my::Random::next() %
                                         possible_jobs_in_home_location.size()];
      if (person->job != nullptr) {
        // надо уволиться со старой работы
        person->job->parent->vacant_places[person->job]++;
      }
      person->job = chosen;
      // уменьшаем число свободных вакансий
      chosen->parent->vacant_places[chosen]--;
      person->job_time = 0;
    }
  }
}
bool GlobalTarget::executable(const Human *person) const {
  // глобальная цель исполнима, если у неё есть исполнимые локальные цели, с
  // которых набирается пул тегов
  auto unclosed_tags = this->tags;
  // удаляем выполненные теги
  for (const auto &i : this->targets_executed)
    for (const auto &j : i->get_tags())
      unclosed_tags.erase(j);
  // проверяем, можем ли мы закрыть все теги, выполнив доступные действия
  for (const auto &i : this->targets_possible)
    if (i->executable(person))
      for (const auto &j : i->get_tags())
        unclosed_tags.erase(j);
  // если все теги закрыты, то глобальная цель объективно достижима
  return unclosed_tags.size() == 0;
}
bool LocalTarget::executable(const Human *person) const {
  // локальная цель исполнима, если исполним пул действий, с которых
  // набираются необходимые теги
  auto unclosed_tags = this->tags;
  // удаляем выполненные теги
  for (const auto &i : this->actions_executed)
    for (const auto &j : i->get_tags())
      unclosed_tags.erase(j);
  // проверяем, можем ли мы закрыть все теги, выполнив доступные действия
  for (const auto &i : this->actions_possible)
    if (i->executable(person))
      for (const auto &j : i->get_tags())
        unclosed_tags.erase(j);
  // если все теги закрыты, то локальная цель объективно достижима
  return unclosed_tags.size() == 0;
}
bool Action::executable(const Human *person) const {
  const vector<string> comparisons1 = {">", "<", "="};
  const vector<string> comparisons2 = {"<>", ">=", "<="};
  for (const auto &rule : rules) {
    string comparison = "";
    for (const auto &i : comparisons2) {
      if (rule.first.find(i) != rule.first.npos) {
        comparison = i;
        break;
      }
    }
    if (comparison.length() == 0)
      for (const auto &i : comparisons1) {
        if (rule.first.find(i) != rule.first.npos) {
          comparison = i;
          break;
        }
      }
    vector<string> sub;
    if (comparison.length()) {
      sub = split(rule.first, comparison);
      if (sub.size() != 2)
        throw(invalid_argument(
            "Comparison value can be operated just with 2 operands!"));
      vector<int> values(sub.size(), 0);
      for (size_t i = 0; i < sub.size(); i++) {
        if (is_natural(sub[i]))
          values[i] = stoi(sub[i]);
        else {
          // это какая-то метрика
          // пока я обрабатываю только "cash", т.е. бюджет семьи и "job_time"
          // -- время на работе
          if (sub[i] == "cash") {
            values[i] = person->money;
            for (const auto &sp : person->family)
              values[i] += sp.first->money;
          }
          if (sub[i] == "job_time") {
            values[i] = person->job_time;
          }
        }
        if (!compare(values[0], comparison, values[1]))
          return false;
      }
    } else {
      // проверяем наличие item у агента
      if (!(person->items.count(rule.first) &&
            person->items.at(rule.first) >= rule.second))
        return false;
    }
  }
  // проверяем, что есть в достаточном числе расходники
  for (const auto &i : removable_items)
    if (person->items.count(i.first) == 0 ||
        person->items.at(i.first) < i.second)
      return false;
  return true;
}

int main() {
  cerr << "GT:\n";
  for (const auto &i : GLOBAL_TARGETS) {
    cerr << i->get_name() << ":";
    for (const auto &j : i->get_tags())
      cerr << " " << j;
    cerr << endl;
  }
  cerr << "LT:\n";
  for (const auto &i : LOCAL_TARGETS) {
    cerr << i->get_name() << " :";
    for (const auto &j : i->get_tags())
      cerr << " " << j;
    cerr << endl;
  }
  cerr << "ACT:\n";
  for (const auto &i : ACTIONS) {
    cerr << i->get_name() << "[";
    cerr << i->get_price() << ", ";
    cerr << i->get_time_to_execute();
    cerr << "]:";
    for (const auto &j : i->get_tags())
      cerr << " " << j;
    cerr << endl;
  }

  // Создаем город
  Location city;

  // Создаем вакансию и работу
  Job *company = new Job();
  company->home_location = &city;
  Vacancy *vacancy = new Vacancy();
  vacancy->parent = company;
  vacancy->payment = 50000; // Зарплата 50 000 рублей в месяц
  company->vacant_places[vacancy] = 100;
  city.jobs.insert(company);

  // Создаем 5 жителей
  vector<Human *> people;
  for (int i = 0; i < 100; i++) {
    Human *h = new Human(unordered_set<Human *>(), &city);
    h->money = 10000; // Стартовый капитал
    people.push_back(h);
    city.humans.insert(h);
  }

  const uint64_t hours_per_month = 30 * 24;        // 1 месяц в часах
  const uint64_t hours_per_year = 365 * 24;        // 1 год в часах
  const uint64_t total_hours = 1 * hours_per_year; // 1 год в часах

  double iterate_timer = 0;
  // Цикл симуляции
  for (uint64_t hour = 0; hour < total_hours; hour++) {
    // Статистика каждый месяц
    if (hour % hours_per_month == 0) {
      int month = hour / hours_per_month;
      int alive_count = 0;
      double total_age = 0;
      double total_money = 0;

      for (auto &p : people) {
        if (!p->dead) {
          alive_count++;
          total_age += p->age;
          total_money += p->money;
        }
      }

      cout << "Month " << month << ": alive = " << alive_count;
      if (alive_count > 0) {
        cout << " | avg_age = " << total_age / alive_count
             << " | avg_money = " << total_money / alive_count;
      }
      cout << endl;
    }

    const double t0 = omp_get_wtime();
    // Обработка каждого жителя
    for (auto &p : people) {
      if (!p->dead) {
        p->iterate_hour();
      }
    }
    const double t1 = omp_get_wtime();
    iterate_timer += t1 - t0;

    Tick::tick(); // Увеличиваем глобальный счётчик времени
  }

  for (const auto &i : people) {
    cerr << i << ": " << i->splashes.size() << endl;
    for (const auto &j : i->splashes) {
      cerr << "  " << j.appear_time << ", " << j.name << ", " << j.life_length
           << endl;
      for (const auto &k : j.tags)
        cerr << "    " << k << endl;
    }
  }

  cerr << "iterate: " << iterate_timer << endl;

  // Освобождение памяти
  for (auto p : people)
    delete p;
  delete vacancy;
  delete company;

  return 0;
}