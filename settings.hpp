/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_SETTINGS_HPP
#define C4S_SETTINGS_HPP

#include <sstream>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace c4s {

namespace settings {
class section;
class item;
};

// ...............................................
class configuration
{
  public:
    enum class FORMAT
    {
        FLAT,
        JSON
    };

    configuration();
    configuration(FORMAT type, std::istream& input);
    ~configuration();

    bool read(FORMAT type, std::istream& input);
    settings::section* create_section(const std::string name);
    settings::section* get_section(const std::string& name);

    std::string get_string(const std::string& section, const std::string& name);
    bool get_value(const std::string& section, const std::string& name, std::string& val);
    bool get_value(const std::string& section, const std::string& name, uint64_t& val);
    bool get_value(const std::string& section, const std::string& name, float& val);
    bool get_value(const std::string& section, const std::string& name, bool& val);
    bool is(const std::string& section, const std::string& name);

    std::list<settings::section*>::iterator begin() { return sections.begin(); }
    std::list<settings::section*>::iterator end() { return sections.end(); }

  protected:
    settings::item* find(const std::string& section, const std::string& name);
    std::list<settings::section*> sections;
};

namespace settings {

enum class TYPE
{
    STR,
    LONG,
    FLOAT,
    BOOL
};

class item
{
    friend class section;

  public:
    item(TYPE _t)
      : type(_t)
    {}
    virtual ~item() {}
    TYPE get_type() { return type; }
    virtual void print(std::ostream&) const = 0;

  private:
    TYPE type;
};

typedef std::unordered_map<std::string, settings::item*> setting_map;

// ...............................................
class str_item : public item
{
  public:
    str_item(const std::string& val)
      : item(TYPE::STR)
      , value(val)
    {}
    str_item(const str_item& original)
      : item(TYPE::STR)
    {
        value = original.value;
    }
    void print(std::ostream& os) const { os << value; }
    std::string value;
};
// ...............................................
class integer_item : public item
{
  public:
    integer_item(uint64_t val)
      : item(TYPE::LONG)
      , value(val)
    {}
    integer_item(const integer_item& original)
      : item(TYPE::LONG)
    {
        value = original.value;
    }
    void print(std::ostream& os) const { os << value; }
    uint64_t value;
};
// ...............................................
class float_item : public item
{
  public:
    float_item(float val)
      : item(TYPE::FLOAT)
      , value(val)
    {}
    float_item(const float_item& original)
      : item(TYPE::FLOAT)
    {
        value = original.value;
    }
    void print(std::ostream& os) const { os << value; }
    float value;
};
// ...............................................
class bool_item : public item
{
  public:
    bool_item(bool val)
      : item(TYPE::BOOL)
      , value(val)
    {}
    bool_item(const bool_item& original)
      : item(TYPE::BOOL)
    {
        value = original.value;
    }
    void print(std::ostream& os) const { os << (value ? "true" : "false"); }
    bool value;
};

// ...............................................
//! Parse time data structure
struct json_parse_data
{
    json_parse_data()
    {
        pos = 0;
        read_level = 0;
        array_item = -1;
    }
    void level_up()
    {
        read_key += '.';
        read_level++;
    }
    void level_down()
    {
        read_key.pop_back();
        read_level--;
    }
    bool is_array() { return array_item >= 0 ? true : false; }
    std::string get_key()
    {
        if (array_item >= 0) {
            std::ostringstream ss;
            ss << read_key << '[' << array_item << ']';
            array_item++;
            return ss.str();
        } else
            return read_key;
    }
    size_t pos;
    std::string read_key;
    int read_level;
    int array_item;
};
// ...............................................
class section
{
    friend class c4s::configuration;

  public:
    ~section();
    bool read(configuration::FORMAT type, std::istream& input);

    std::string get_name();
    bool is_name(const std::string& _name);
    void get_subkeys(const std::string& name, std::vector<std::string>& keys);
    void get_values(const std::string& name, std::vector<std::string>& values);

    setting_map items;

  protected:
    section();
    section(const std::string& name);
    section(const std::string& name, configuration::FORMAT type, std::istream& input);

    bool read_flat(std::istream& input);
    bool read_json(std::istream& input, json_parse_data& pd);

    std::string name;
};

}; // c4s::settings namespace

}; // c4s namespace

inline std::ostream&
operator<<(std::ostream& os, const c4s::settings::item* im)
{
    im->print(os);
    return os;
}

#endif
