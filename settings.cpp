/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include "config.hpp"
#include "exception.hpp"
#include "logger.hpp"
#include "settings.hpp"

using namespace c4s;
using namespace std;

namespace c4s {

namespace settings {
// -------------------------------------------------------------------------------------------------
section::section() {}
// -------------------------------------------------------------------------------------------------
section::section(const std::string& _name)
  : name(_name)
{}
// -------------------------------------------------------------------------------------------------
section::section(const std::string& _name, configuration::FORMAT format, std::istream& input)
  : name(_name)
{
    if (!read(format, input))
        throw c4s_exception("section::section - syntax or read failure.");
}
// -------------------------------------------------------------------------------------------------
section::~section()
{
    for (auto item : items) {
        delete item.second;
    }
}
// -------------------------------------------------------------------------------------------------
std::string
section::get_name()
{
    return name;
}
// -------------------------------------------------------------------------------------------------
bool
section::is_name(const std::string& _name)
{
    return name.compare(_name) == 0 ? true : false;
}
// -------------------------------------------------------------------------------------------------
void
section::get_subkeys(const std::string& name, std::vector<std::string>& keys)
{
    vector<string>::iterator kit;
    string needle;
    size_t pos = name.find('*');
    if (pos == string::npos)
        needle = name;
    else
        needle = name.substr(0, pos);

    size_t len = needle.size();
    for (auto item : items) {
        if (!item.first.compare(0, len, needle)) {
            pos = item.first.find('.', len);
            string subkey =
                pos == string::npos ? item.first.substr(len) : item.first.substr(len, pos - len);
            for (kit = keys.begin(); kit != keys.end(); kit++) {
                if (!kit->compare(subkey))
                    break;
            }
            if (kit == keys.end())
                keys.push_back(subkey);
        }
    }
}
// -------------------------------------------------------------------------------------------------
void
section::get_values(const std::string& name, std::vector<std::string>& values)
{
    setting_map::iterator si;
    size_t pos = name.find('*');
    if (pos == string::npos) {
        return;
    }

    size_t ndx = 0;
    do {
        ostringstream ns;
        ns << name.substr(0, pos);
        ns << ndx;
        ns << name.substr(pos + 1);
        for (si = items.begin(); si != items.end(); si++) {
            if (!si->first.compare(ns.str()) && si->second->get_type() == TYPE::STR) {
                values.push_back(static_cast<str_item*>(si->second)->value);
                break;
            }
        }
        ndx++;
    } while (si != items.end());
}
// -------------------------------------------------------------------------------------------------
bool
section::read_flat(std::istream& input)
{
    char line[256];
    do {
        input.getline(line, sizeof(line));
        char* es = strchr(line, '=');
        if (es) {
            *es = 0;
            // Trim name
            char* end = es - 1;
            while (*end == ' ' || *end == '\t') {
                *end = 0;
                end--;
            }
            // Trim value
            es++;
            while (*es == ' ' || *es == '\t') {
                es++;
            }
            items[string(line)] = new str_item(es);
        }
    } while (!input.eof());
    return true;
}
// -------------------------------------------------------------------------------------------------
bool
section::read_json(std::istream& input, json_parse_data& pd)
{
    enum
    {
        START,
        KEY,
        VALUE,
        VSTR,
        VNUM,
        KEYWORD,
        END
    } state;
    enum
    {
        LONG,
        FLOAT
    } numtype;
    char ch;
    string original_read_key(pd.read_key);
    string value;

    state = pd.is_array() ? VALUE : START;
    while (!input.eof()) {
        input.read(&ch, 1);
        switch (state) {
        case START:
            if (ch == '"')
                state = KEY;
            else if (ch == '}') {
                CS_VAPRT_ERRO("section::read_json - expected key at %ld", pd.pos);
                return false;
            }
            break;
        case KEY:
            if (ch == '"') {
                state = VALUE;
                value.clear();
            } else
                pd.read_key += ch;
            break;
        case VALUE:
            if (ch == '"')
                state = VSTR;
            else if ((ch >= '0' && ch <= '9') || ch == '-') {
                numtype = LONG;
                state = VNUM;
                input.seekg(-1, ios_base::cur);
            } else if (ch == 'n' || ch == 't' || ch == 'f') {
                if (pd.is_array()) {
                    CS_PRINT_ERRO("section::read_json - arrays of true/false, null not supported.");
                    return false;
                }
                state = KEYWORD;
                input.seekg(-1, ios_base::cur);
            } else if (ch == '[') {
                if (pd.is_array()) {
                    CS_PRINT_ERRO("section::read_json - nested arrays not supported.");
                    return false;
                }
                pd.array_item = 0;
                pd.level_up();
                if (!read_json(input, pd))
                    return false;
                state = END;
            } else if (ch == '{') {
                if (pd.is_array()) {
                    CS_PRINT_ERRO("section::read_json - objects within array not supported.");
                    return false;
                }
                pd.level_up();
                if (!read_json(input, pd))
                    return false;
                state = END;
            } else if (ch == '}') {
                CS_VAPRT_ERRO("section::read_json - expected value for key %s",
                              pd.read_key.c_str());
                return false;
            }
            break;
        case VSTR:
            if (ch == '"') {
                items[pd.get_key()] = new str_item(value);
                state = END;
            } else
                value += ch;
            break;
        case VNUM:
            if (ch == ',' || ch == '}' || ch == '\t' || ch == ' ' || ch == '\n' || ch == ']') {
                try {
                    if (numtype == LONG) {
                        items[pd.get_key()] = new integer_item(stoul(value));
                    } else
                        items[pd.get_key()] = new float_item(stof(value));
                    if (ch == ']') {
                        pd.level_down();
                        pd.array_item = -1;
                        return true;
                    }
                    if (ch == ',' || ch == '}')
                        input.seekg(-1, ios_base::cur);
                    state = END;
                } catch (const std::invalid_argument& ia) {
                    CS_VAPRT_ERRO("section::read_json - number format failed for: '%s'",
                                  value.c_str());
                    return false;
                } catch (const std::out_of_range& oor) {
                    CS_VAPRT_ERRO("section::read_json - number out of range: '%s'", value.c_str());
                    return false;
                }
            } else if (ch == '.') {
                numtype = FLOAT;
                value += ch;
            } else
                value += ch;
            break;
        case KEYWORD:
            if (ch == ',' || ch == '}') {
                if (!value.compare("true"))
                    items[pd.read_key] = new bool_item(true);
                else if (!value.compare("false"))
                    items[pd.read_key] = new bool_item(false);
                else if (value.compare("null")) {
                    CS_VAPRT_ERRO("section::read - unknown json keyword: %s", value.c_str());
                    return false;
                }
                input.seekg(-1, ios_base::cur);
                state = END;
            } else if (ch != '\t' && ch != ' ' && ch != '\n')
                value += ch;
            break;
        case END:
            if (ch == ',') {
                if (pd.is_array()) {
                    state = VALUE;
                    value.clear();
                } else {
                    state = START;
                    pd.read_key = original_read_key;
                }
            } else if (ch == '}') {
                pd.level_down();
                pd.read_key = original_read_key;
                return true;
            } else if (ch == ']') {
                pd.level_down();
                pd.array_item = -1;
                return true;
            }
            break;
        }
        pd.pos++;
    }
    return true;
}
// -------------------------------------------------------------------------------------------------
bool
section::read(configuration::FORMAT format, std::istream& input)
{
    if (format == configuration::FORMAT::FLAT) {
        return read_flat(input);
    } else if (format == configuration::FORMAT::JSON) {
        json_parse_data pd;
        return read_json(input, pd);
    }
    CS_PRINT_ERRO("section::read - unsupported format");
    return false;
}

}; // c4s::settings namespace

// =================================================================================================
configuration::configuration()
{
    // Intentionally empty
}

configuration::configuration(configuration::FORMAT format, std::istream& input)
{
    if (!read(format, input))
        throw c4s_exception("configuration::configuration - syntax or read failure.");
}
configuration::~configuration()
{
    list<settings::section*>::iterator ss;
    for (ss = sections.begin(); ss != sections.end(); ss++) {
        delete (*ss);
    }
}
bool
configuration::read(configuration::FORMAT format, std::istream& input)
{
    settings::section* ss;

    if (format == configuration::FORMAT::FLAT) {
        ss = new settings::section("general");
        if (!ss->read(format, input))
            return false;
        sections.push_back(ss);
    } else if (format == configuration::FORMAT::JSON) {
        enum
        {
            START,
            L0,
            SNAME,
            L1
        } state = START;
        char ch;
        string sname;
        while (!input.eof()) {
            input >> ch;
            switch (state) {
            case START:
                if (ch == '{')
                    state = L0;
                break;
            case L0:
                if (ch == '"') {
                    sname.clear();
                    state = SNAME;
                }
                break;
            case SNAME:
                if (ch == '"')
                    state = L1;
                else
                    sname += ch;
                break;
            case L1:
                if (ch == '{') {
                    ss = new settings::section(sname);
                    sections.push_back(ss);
                    if (!ss->read(format, input)) {
                        delete ss;
                        return false;
                    }
                    state = L0;
                }
                break;
            }
        }
    } else {
        CS_PRINT_ERRO("configuration::read - Unknown format.");
        return false;
    }
    return true;
}

settings::section*
configuration::get_section(const std::string& name)
{
    list<settings::section*>::iterator ss;
    for (ss = sections.begin(); ss != sections.end(); ss++) {
        if ((*ss)->is_name(name))
            return *ss;
    }
    return 0;
}

settings::item*
configuration::find(const std::string& section, const std::string& name)
{
    // find sections
    list<settings::section*>::iterator ss;
    for (ss = sections.begin(); ss != sections.end(); ss++) {
        if ((*ss)->is_name(section))
            break;
    }
    if (ss == sections.end())
        return 0;
    // Find item
    auto si = (*ss)->items.find(name);
    if (si == (*ss)->items.end())
        return 0;
    return si->second;
}

std::string
configuration::get_string(const std::string& section, const std::string& name)
{
    settings::item* si = find(section, name);
    if (!si || si->get_type() != settings::TYPE::STR)
        return string();
    return static_cast<settings::str_item*>(si)->value;
}
bool
configuration::get_value(const std::string& section, const std::string& name, std::string& val)
{
    settings::item* si = find(section, name);
    if (!si || si->get_type() != settings::TYPE::STR)
        return false;
    val = static_cast<settings::str_item*>(si)->value;
    return true;
}
bool
configuration::get_value(const std::string& section, const std::string& name, uint64_t& val)
{
    settings::item* si = find(section, name);
    if (!si || si->get_type() != settings::TYPE::LONG)
        return false;
    val = static_cast<settings::integer_item*>(si)->value;
    return true;
}
bool
configuration::get_value(const std::string& section, const std::string& name, float& val)
{
    settings::item* si = find(section, name);
    if (!si || si->get_type() != settings::TYPE::FLOAT)
        return false;
    val = static_cast<settings::float_item*>(si)->value;
    return true;
}
bool
configuration::get_value(const std::string& section, const std::string& name, bool& val)
{
    settings::item* si = find(section, name);
    if (!si || si->get_type() != settings::TYPE::BOOL)
        return false;
    val = static_cast<settings::bool_item*>(si)->value;
    return true;
}
bool
configuration::is(const std::string& section, const std::string& name)
{
    settings::item* si = find(section, name);
    if (!si || si->get_type() != settings::TYPE::BOOL)
        return false;
    return static_cast<settings::bool_item*>(si)->value;
}

}; // c4s namespace
