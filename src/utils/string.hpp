#pragma once

#include <string>
#include <vector>

std::vector<std::string> split_text(const std::string &text, char delimiter);

std::string join_vector(const std::vector<std::string> &vec, char delimiter);
std::string join_vector(const std::vector<std::string> &vec);
