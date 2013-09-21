// disable warnings of the MSVS compiler
#include <boost/config/warning_disable.hpp>
// include the unit testing framework
#define BOOST_TEST_MODULE BASE64 Test
#include <boost/test/unit_test.hpp>
// include the generic base64 encoding support
#include <boost/network/utils/base64/encode.hpp>
#include <boost/network/utils/base64/encode-io.hpp>
// include the locally needed headers
#include <boost/array.hpp>
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>

using namespace boost::network::utils;

// proves that all public functions are compilable; the result check
// is very minimum here, so that the test doesn't look so stupid ;-)
BOOST_AUTO_TEST_CASE(interface_test) {
    std::string result;
    base64::state<char> state;

    // check a string literal as input
    result = base64::normal::encode<char>("abc");
    BOOST_CHECK_EQUAL(result, "YWJj");

    result.clear();
    base64::normal::encode("abc", std::back_inserter(result));
    BOOST_CHECK_EQUAL(result, "YWJj");

    result.clear();
    base64::normal::encode("abc", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YWJj");

    // check a std::string as input
    std::string input("abc");

    result = base64::normal::encode<char>(input);
    BOOST_CHECK_EQUAL(result, "YWJj");

    result.clear();
    base64::normal::encode(input, std::back_inserter(result));
    BOOST_CHECK_EQUAL(result, "YWJj");

    result.clear();
    base64::normal::encode(input.begin(), input.end(), std::back_inserter(result));
    BOOST_CHECK_EQUAL(result, "YWJj");

    result.clear();
    base64::normal::encode(input, std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YWJj");

    result.clear();
    base64::normal::encode(input.begin(), input.end(), std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YWJj");

    // check an array of chars as input
    char char_array[] = { 'a', 'b', 'c' };

    result = base64::normal::encode<char>(char_array);
    BOOST_CHECK_EQUAL(result, "YWJj");

    // check a boost::array of chars as input
    boost::array<char, 3> char_boost_array = { { 'a', 'b', 'c' } };

    result = base64::normal::encode<char>(char_boost_array);
    BOOST_CHECK_EQUAL(result, "YWJj");

    // check a std::vector of chars as input
    std::vector<char> char_vector(char_array, char_array + 3);

    result = base64::normal::encode<char>(char_vector);
    BOOST_CHECK_EQUAL(result, "YWJj");

    // check that base64::encode_rest is compilable and callable
    result.clear();
    base64::normal::encode_rest(std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "");

    // check that the iostream interface is compilable and callable
    std::ostringstream output;

    output << base64::io::normal::encode("abc") <<
              base64::io::normal::encode(input.begin(), input.end()) <<
              base64::io::normal::encode(char_array) <<
              base64::io::normal::encode(char_boost_array) <<
              base64::io::normal::encode(char_vector) <<
              base64::io::normal::encode_rest<char>;
    BOOST_CHECK_EQUAL(output.str(), "YWJjYWJjYWJjYWJjYWJj");
}

// checks that functions encoding a single chunk append the correct padding
// if the input byte count is not divisible by 3
BOOST_AUTO_TEST_CASE(padding_test) {
    std::string result;

    result = base64::normal::encode<char>("");
    BOOST_CHECK_EQUAL(result, "");
    result = base64::normal::encode<char>("a");
    BOOST_CHECK_EQUAL(result, "YQ==");
    result = base64::normal::encode<char>("aa");
    BOOST_CHECK_EQUAL(result, "YWE=");
    result = base64::normal::encode<char>("aaa");
    BOOST_CHECK_EQUAL(result, "YWFh");
}

// check that functions using encoding state interrupt and resume encoding
// correcly if the byte count of the partial input is not divisible by 3
BOOST_AUTO_TEST_CASE(state_test) {
    base64::state<char> state;
    std::string result;

    // a newly constructed state must be empty
    BOOST_CHECK(state.empty());

    // check encoding empty input; including the state value
    base64::normal::encode("", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "");
    BOOST_CHECK(state.empty());

    // check one third of quantum which needs two character padding;
    // including how the state develops when encoded by single character
    result.clear();
    base64::normal::encode("a", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "Y");
    BOOST_CHECK(!state.empty());
    base64::normal::encode_rest(std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YQ==");
    BOOST_CHECK(state.empty());

    // check two thirds of quantum which needs one character padding;
    // including how the state develops when encoded by single character
    result.clear();
    base64::normal::encode("a", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "Y");
    BOOST_CHECK(!state.empty());
    base64::normal::encode("a", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YW");
    BOOST_CHECK(!state.empty());
    base64::normal::encode_rest(std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YWE=");
    BOOST_CHECK(state.empty());

    // check a complete quantum which needs no padding; including
    // how the state develops when encoded by single character
    result.clear();
    base64::normal::encode("a", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "Y");
    BOOST_CHECK(!state.empty());
    base64::normal::encode("a", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YW");
    BOOST_CHECK(!state.empty());
    base64::normal::encode("a", std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YWFh");
    BOOST_CHECK(state.empty());
    base64::normal::encode_rest(std::back_inserter(result), state);
    BOOST_CHECK_EQUAL(result, "YWFh");
    BOOST_CHECK(state.empty());

    // check that forced clearing the state works
    result.clear();
    base64::normal::encode("a", std::back_inserter(result), state);
    BOOST_CHECK(!state.empty());
    state.clear();
    BOOST_CHECK(state.empty());
}

// checks that the base64 output can be returned as wchar_t too
BOOST_AUTO_TEST_CASE(wide_character_test) {
    std::wstring result;

    // check the iterator interface
    result = base64::normal::encode<wchar_t>("abc");
    BOOST_CHECK(result == L"YWJj");
    result = base64::normal::encode<wchar_t>(std::string("abc"));
    BOOST_CHECK(result == L"YWJj");

    // check the stream manipulator interface
    std::wostringstream output;
    output << base64::io::normal::encode("abc") <<
              base64::io::normal::encode_rest<char>;
    BOOST_CHECK(output.str() == L"YWJj");
}

// check that one of the two built-in alphabets can be selected
BOOST_AUTO_TEST_CASE(alphabet_test) {
    unsigned char input[] = { 0xfb, 0xf0 };
    std::string result;

    result = base64::normal::encode<char>(input);
    BOOST_CHECK_EQUAL(result, "+/A=");
    result = base64::url::encode<char>(input);
    BOOST_CHECK_EQUAL(result, "-_A=");
}

// check that the output padding can be disabled
BOOST_AUTO_TEST_CASE(no_padding_test) {
    typedef struct base64::encoder<
        base64::default_alphabet,
        base64::no_padding,
        base64::no_line_breaks
        > custom;
    std::string result;

    result = custom::encode<char>("");
    BOOST_CHECK_EQUAL(result, "");
    result = custom::encode<char>("a");
    BOOST_CHECK_EQUAL(result, "YQ");
    result = custom::encode<char>("aa");
    BOOST_CHECK_EQUAL(result, "YWE");
    result = custom::encode<char>("aaa");
    BOOST_CHECK_EQUAL(result, "YWFh");
}

// check that the line length can be configured
BOOST_AUTO_TEST_CASE(line_breaks_test) {
    std::string result;

    // encode 100 8-bit units to 134 6-bit units and check how
    // many line breaks have been inserted into the output

    // no line break by default
    result = base64::normal::encode<char>("0000000001"
                                          "1111111112"
                                          "2222222223"
                                          "3333333334"
                                          "4444444445"
                                          "5555555556"
                                          "6666666667"
                                          "7777777778"
                                          "8888888889"
                                          "9999999990");
    BOOST_CHECK_EQUAL(result.size(), 134 + 2); // padding
    BOOST_CHECK(result.find('\n') == std::string::npos);

    // one line break at the character 76 of 133 for MIME
    result = base64::mime::encode<char>("0000000001"
                                        "1111111112"
                                        "2222222223"
                                        "3333333334"
                                        "4444444445"
                                        "5555555556"
                                        "6666666667"
                                        "7777777778"
                                        "8888888889"
                                        "9999999990");
    BOOST_CHECK_EQUAL(result.size(), 134 + 2 + 1); // padding, eoln
    BOOST_CHECK(result.find('\n') == 76);
    BOOST_CHECK(result.find('\n', 76 + 1) == std::string::npos);

    // two line breaks at the characters 64 and 129 of 133 for PEM
    result = base64::pem::encode<char>("0000000001"
                                       "1111111112"
                                       "2222222223"
                                       "3333333334"
                                       "4444444445"
                                       "5555555556"
                                       "6666666667"
                                       "7777777778"
                                       "8888888889"
                                       "9999999990");
    BOOST_CHECK_EQUAL(result.size(), 134 + 2 + 2); // padding, 2 eolns
    BOOST_CHECK(result.find('\n') == 64);
    BOOST_CHECK(result.find('\n', 64 + 1) == 64 + 1 + 64);
    BOOST_CHECK(result.find('\n', 64 + 1 + 64 + 1) == std::string::npos);
}

// checks that the base64-io manipulators are compilable and work
BOOST_AUTO_TEST_CASE(io_test) {
    std::ostringstream output;

    // check a complete quantum where no state has to be remembered
    output << base64::io::normal::encode("abc") <<
              base64::io::normal::encode_rest<char>;
    BOOST_CHECK_EQUAL(output.str(), "YWJj");

    // check that encode_rest clears the state
    output.str("");
    output << base64::io::normal::encode("a");
    BOOST_CHECK(!base64::io::empty_state<char>(output));
    output << base64::io::normal::encode_rest<char>;
    BOOST_CHECK(base64::io::empty_state<char>(output));

    // check that forced clearing the state works
    output.str("");
    output << base64::io::normal::encode("a");
    BOOST_CHECK(!base64::io::empty_state<char>(output));
    base64::io::clear_state<char>(output);
    BOOST_CHECK(base64::io::empty_state<char>(output));

    // check one third of quantum which has to be remembered in state
    output.str("");
    output << base64::io::normal::encode("a") <<
              base64::io::normal::encode("bc") <<
              base64::io::normal::encode_rest<char>;
    BOOST_CHECK_EQUAL(output.str(), "YWJj");

    // check two thirds of quantum which have to be remembered in state.
    output.str("");
    output << base64::io::normal::encode("ab") <<
              base64::io::normal::encode("c") <<
              base64::io::normal::encode_rest<char>;
    BOOST_CHECK_EQUAL(output.str(), "YWJj");
}
