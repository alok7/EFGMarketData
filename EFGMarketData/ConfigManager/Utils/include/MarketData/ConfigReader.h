#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#ifndef MAX_LINE
#define MAX_LINE 2048
#endif


#ifndef MULTILINE_ALLOWED
#define MULTILINE_ALLOWED 1
#endif

#ifndef CONFIG_BOM_ALLOWED
#define CONFIG_BOM_ALLOWED 1
#endif

#ifndef INLINE_COMMENTS
#define INLINE_COMMENTS 1
#endif
  
#ifndef COMMENT_PREFIXES
#define COMMENT_PREFIXES ";"
#endif

#ifndef STACK_USE
#define STACK_USE 1
#endif

#ifndef STOP_ON_FIRST_ERROR
#define STOP_ON_FIRST_ERROR 0
#endif

typedef int (*config_handler)(void* user, const char* section, const char* name, const char* value);
typedef char* (*config_reader)(char* str, int num, void* stream);

//comments with ';'
  
int config_parse(const char* filename, config_handler handler, void* user);
int config_parse_file(FILE* file, config_handler handler, void* user);
int config_parse_stream(config_reader reader, void* stream, config_handler handler,void* user);

#ifdef __cplusplus
}
#endif


#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#if !STACK_USE
#include <stdlib.h>
#endif

#define MAX_SECTION 50
#define MAX_NAME 50

inline static char* rstrip(char* s)
{
    char* p = s + strlen(s);
    while (p > s && isspace((unsigned char)(*--p)))
        *p = '\0';
    return s;
}

inline static char* lskip(const char* s)
{
    while (*s && isspace((unsigned char)(*s)))
        s++;
    return (char*)s;
}

inline static char* find_chars_or_comment(const char* s, const char* chars)
{
#if INLINE_COMMENTS
    int was_space = 0;
    while (*s && (!chars || !strchr(chars, *s)) &&
           !(was_space && strchr(COMMENT_PREFIXES, *s))) {
        was_space = isspace((unsigned char)(*s));
        s++;
    }
#else
    while (*s && (!chars || !strchr(chars, *s))) {
        s++;
    }
#endif
    return (char*)s;
}

inline static char* strncpy_append_null(char* dest, const char* src, size_t size)
{
    strncpy(dest, src, size);
    dest[size - 1] = '\0';
    return dest;
}

inline int config_parse_stream(config_reader reader, void* stream, config_handler handler,
                     void* user)
{
#if STACK_USE
    char line[MAX_LINE];
#else
    char* line;
#endif
    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";

    char* start;
    char* end;
    char* name;
    char* value;
    int lineno = 0;
    int error = 0;

#if !STACK_USE
    line = (char*)malloc(MAX_LINE);
    if (!line) {
        return -2;
    }
#endif

    /* Scan through stream line by line */
    while (reader(line, MAX_LINE, stream) != NULL) {
        lineno++;

        start = line;
#if CONFIG_BOM_ALLOWED
        if (lineno == 1 && (unsigned char)start[0] == 0xEF &&
                           (unsigned char)start[1] == 0xBB &&
                           (unsigned char)start[2] == 0xBF) {
            start += 3;
        }
#endif
        start = lskip(rstrip(start));

        if (*start == ';' || *start == '#') {
        }
#if MULTILINE_ALLOWED
        else if (*prev_name && *start && start > line) {

#if INLINE_COMMENTS
        end = find_chars_or_comment(start, NULL);
        if (*end)
            *end = '\0';
        rstrip(start);
#endif
            if (!handler(user, section, prev_name, start) && !error)
                error = lineno;
        }
#endif
        else if (*start == '[') {
            end = find_chars_or_comment(start + 1, "]");
            if (*end == ']') {
                *end = '\0';
                strncpy_append_null(section, start + 1, sizeof(section));
                *prev_name = '\0';
            }
            else if (!error) {
                error = lineno;
            }
        }
        else if (*start) {
           
            end = find_chars_or_comment(start, "=:");
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = lskip(end + 1);
#if INLINE_COMMENTS
                end = find_chars_or_comment(value, NULL);
                if (*end)
                    *end = '\0';
#endif
                rstrip(value);

                strncpy_append_null(prev_name, name, sizeof(prev_name));
                if (!handler(user, section, name, value) && !error)
                    error = lineno;
            }
            else if (!error) {
                error = lineno;
            }
        }

#if STOP_ON_FIRST_ERROR
        if (error)
            break;
#endif
    }

#if !STACK_USE
    free(line);
#endif

    return error;
}

inline int config_parse_file(FILE* file, config_handler handler, void* user)
{
    return config_parse_stream((config_reader)fgets, file, handler, user);
}

inline int config_parse(const char* filename, config_handler handler, void* user)
{
    FILE* file;
    int error;

    file = fopen(filename, "r");
    if (!file)
        return -1;
    error = config_parse_file(file, handler, user);
    fclose(file);
    return error;
}

#endif /* __CONFIG_H__ */


#ifndef __CONFIGREADER_H__
#define __CONFIGREADER_H__

#include <map>
#include <unordered_map>
#include <set>
#include <string>

// Read file into key/value pairs
class CONFIGReader
{
public:
    CONFIGReader() {};

    CONFIGReader(std::string filename);
    CONFIGReader(FILE *file);
    int ParseError() const;

    const std::set<std::string>& Sections() const;
    std::string Get(std::string section, std::string name,
                    std::string default_value) const;

    long GetInteger(std::string section, std::string name, long default_value) const;
    double GetReal(std::string section, std::string name, double default_value) const;
    float GetFloat(std::string section, std::string name, float default_value) const;
    bool GetBoolean(std::string section, std::string name, bool default_value) const;
    std::unordered_map<std::string, std::string> GetMap(std::string section, std::string name) const;
    std::unordered_map<std::string, std::string> GetKeyValues(std::string section) const;

protected:
    int _error;
    std::map<std::string, std::string> _values;
    std::set<std::string> _sections;
    static std::string MakeKey(std::string section, std::string name);
    static int ValueHandler(void* user, const char* section, const char* name,
                            const char* value);
};

#endif  // __CONFIGREADER_H__


#ifndef __CONFIGREADER__
#define __CONFIGREADER__

#include <algorithm>
#include <cctype>
#include <cstdlib>

inline CONFIGReader::CONFIGReader(std::string filename)
{
    _error = config_parse(filename.c_str(), ValueHandler, this);
}

inline CONFIGReader::CONFIGReader(FILE *file)
{
    _error = config_parse_file(file, ValueHandler, this);
}

inline int CONFIGReader::ParseError() const
{
    return _error;
}

inline const std::set<std::string>& CONFIGReader::Sections() const
{
    return _sections;
}

inline std::string CONFIGReader::Get(std::string section, std::string name, std::string default_value) const
{
    std::string key = MakeKey(section, name);
    return _values.count(key) ? _values.at(key) : default_value;
}

inline long CONFIGReader::GetInteger(std::string section, std::string name, long default_value) const
{
    std::string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    long n = strtol(value, &end, 0);
    return end > value ? n : default_value;
}

inline double CONFIGReader::GetReal(std::string section, std::string name, double default_value) const
{
    std::string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    double n = strtod(value, &end);
    return end > value ? n : default_value;
}

inline float CONFIGReader::GetFloat(std::string section, std::string name, float default_value) const
{
    std::string valstr = Get(section, name, "");
    const char* value = valstr.c_str();
    char* end;
    float n = strtof(value, &end);
    return end > value ? n : default_value;
}

inline bool CONFIGReader::GetBoolean(std::string section, std::string name, bool default_value) const
{
    std::string valstr = Get(section, name, "");
    std::transform(valstr.begin(), valstr.end(), valstr.begin(), ::tolower);
    if (valstr == "true" || valstr == "yes" || valstr == "on" || valstr == "1")
        return true;
    else if (valstr == "false" || valstr == "no" || valstr == "off" || valstr == "0")
        return false;
    else
        return default_value;
}

inline std::unordered_map<std::string, std::string> CONFIGReader::GetMap(std::string section, std::string name) const
{
    std::string valstr = Get(section, name, "");
    std::unordered_map<std::string, std::string> mp;
    int strt = 0;
    int end = 0;
    // Repeat till end is reached
    while( strt < valstr.size())
    {
        end = valstr.find(":",strt);
        std::string key = valstr.substr(strt,end-strt);
        strt = end+1;
        end = valstr.find(",",strt);
        if(end == std::string::npos) end = valstr.size();
        std::string value = valstr.substr(strt, end - strt);
        mp[key] = value;
        strt = end+1;
    }
    return mp;
}

inline std::string CONFIGReader::MakeKey(std::string section, std::string name)
{
    std::string key = section + "=" + name;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);
    return key;
}

inline int CONFIGReader::ValueHandler(void* user, const char* section, const char* name,
                            const char* value)
{
    CONFIGReader* reader = (CONFIGReader*)user;
    std::string key = MakeKey(section, name);
    if (reader->_values[key].size() > 0)
        reader->_values[key] += "\n";
    reader->_values[key] += value;
    reader->_sections.insert(section);
    return 1;
}
inline std::unordered_map<std::string, std::string> CONFIGReader::GetKeyValues(std::string section) const
{
  std::unordered_map<std::string, std::string> SectionLookup;
  for(auto p: _values){
    std::string key = (p.first).substr(0, (p.first).find("="));
    if(key==section){
      SectionLookup[(p.first).substr((p.first).find("=")+1)] = p.second; 
    }
  }
  return SectionLookup;
}

#endif  // __CONFIGREADER__

