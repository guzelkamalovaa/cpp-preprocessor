#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

static bool ProcessFile(ifstream& in_stream, ofstream& out_stream, const path& current_file,
                        const vector<path>& include_directories, int& line_number);

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    ifstream in_stream(in_file);
    if (!in_stream.is_open()) {
        return false;
    }

    ofstream out_stream(out_file);
    if (!out_stream.is_open()) {
        return false;
    }

    int line_number = 0;
    return ProcessFile(in_stream, out_stream, in_file, include_directories, line_number);
}

static bool ProcessFile(ifstream& in_stream, ofstream& out_stream, const path& current_file,
                        const vector<path>& include_directories, int& line_number) {
    static const regex include_quote_regex(R"(#\s*include\s*\"([^\"]*)\")");
    static const regex include_angle_regex(R"(#\s*include\s*<([^>]*)>)");

    string line;
    while (getline(in_stream, line)) {
        ++line_number;
        smatch match;

        if (regex_match(line, match, include_quote_regex) || regex_match(line, match, include_angle_regex)) {
            path include_path = match[1].str();
            ifstream include_stream;
            path resolved_path;

            bool is_quote_include = regex_match(line, match, include_quote_regex);
            if (is_quote_include) {
                // Поиск файла относительно текущего файла
                resolved_path = current_file.parent_path() / include_path;
                include_stream.open(resolved_path);
            }

            if (!include_stream.is_open()) {
                for (const path& dir : include_directories) {
                    resolved_path = dir / include_path;
                    include_stream.open(resolved_path);
                    if (include_stream.is_open()) {
                        break;
                    }
                }
            }

            if (!include_stream.is_open()) {
                cout << "unknown include file " << include_path.string() << " at file " 
                     << current_file.string() << " at line " << line_number << endl;
                return false;
            }

            int inner_line_number = 0;
            if (!ProcessFile(include_stream, out_stream, resolved_path, include_directories, inner_line_number)) {
                return false;
            }
            include_stream.close();
        } else {
            out_stream << line << '\n';
        }
    }
    return true;
}

string GetFileContents(string file) {
    ifstream stream(file);
    return {istreambuf_iterator<char>(stream), istreambuf_iterator<char>()};
}