#pragma once

#include <string>
#include <vector>

namespace netconnect::Utils {

/// Returns the current local time formatted as "YYYY-MM-DD HH:MM:SS".
std::string currentTimestamp();

/// Splits a string on the given delimiter character.
std::vector<std::string> split(const std::string& text, char delimiter);

/// Trims leading/trailing whitespace.
std::string trim(const std::string& text);

/// True if the username contains only alphanumerics/underscore and is
/// within a sane length range.
bool isValidUsername(const std::string& username);

} // namespace netconnect::Utils
