// Minimal, dependency-free unit tests for the Utils namespace.
//
// A full framework (GoogleTest/Catch2) was deliberately skipped to keep
// this project buildable with nothing beyond a C++17 compiler and
// CMake - the same reasoning as the rest of the codebase: prefer the
// standard library over a dependency where the standard library gets
// the job done just as well. Each check is a plain assert(); a failed
// assert aborts the process, which CTest reports as a failing test.

#include <cassert>
#include <iostream>

#include "Utils.h"

namespace {

void testSplit() {
    auto tokens = netconnect::Utils::split("/msg Bob hello there", ' ');
    assert(tokens.size() == 4);
    assert(tokens[0] == "/msg");
    assert(tokens[1] == "Bob");
    assert(tokens[3] == "there");

    // Empty input should yield no tokens.
    assert(netconnect::Utils::split("", ' ').empty());
}

void testTrim() {
    assert(netconnect::Utils::trim("  hello  ") == "hello");
    assert(netconnect::Utils::trim("no_whitespace") == "no_whitespace");
    assert(netconnect::Utils::trim("   ") == "");
    assert(netconnect::Utils::trim("\t\nhi\r\n") == "hi");
}

void testIsValidUsername() {
    assert(netconnect::Utils::isValidUsername("Alice"));
    assert(netconnect::Utils::isValidUsername("user_123"));
    assert(!netconnect::Utils::isValidUsername("a"));               // too short
    assert(!netconnect::Utils::isValidUsername("way_too_long_username_here")); // too long
    assert(!netconnect::Utils::isValidUsername("bad name"));        // contains space
    assert(!netconnect::Utils::isValidUsername("bad/name"));        // invalid char
}

void testTimestampFormat() {
    // Format is "YYYY-MM-DD HH:MM:SS" - 19 characters, fixed layout.
    const std::string ts = netconnect::Utils::currentTimestamp();
    assert(ts.size() == 19);
    assert(ts[4] == '-' && ts[7] == '-' && ts[10] == ' ');
    assert(ts[13] == ':' && ts[16] == ':');
}

} // namespace

int main() {
    testSplit();
    testTrim();
    testIsValidUsername();
    testTimestampFormat();

    std::cout << "All Utils tests passed.\n";
    return 0;
}
