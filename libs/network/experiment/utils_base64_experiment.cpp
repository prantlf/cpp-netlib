// include the original, internally used base64 encoding
#include <boost/network/protocol/http/message.hpp>
// include the generic base64 encoding support
#include <boost/network/utils/base64/encode.hpp>
#include <boost/network/utils/base64/encode-io.hpp>
// include the alternative base64 encoding implementations
#include "utils/base64-standalone.hpp"
#include "utils/base64-stateless.hpp"
#include "utils/base64-stateful_buffer.hpp"
#include "utils/base64-stateful_iterator.hpp"
#include "utils/base64-stateful_transform.hpp"
// include the locally needed headers
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>

// the main entry point does nothing; the tests are run by constructors
// of testing classes, which are executed by global variables
int main(int argc, char const * argv[]) {
    return 0;
}

using namespace boost::network::http;
using namespace boost::network::utils;

// support printing the curret test number to the output
static unsigned test_index = 0;

// macros to build names of test classes ans gobal runner variables
#define stringify_internal(x) #x
#define stringify(x) stringify_internal(x)
#define concatenate_internal(x, y) x##y
#define concatenate(x, y) concatenate_internal(x, y)

#ifdef NDEBUG
    // testing block sizes (single im MB, multiple in KB) which let the tests
    // run approximately 5s on my machine in the optimized mode
    #define single_block_size    160
    #define multiple_block_size  320
    #define multiple_block_count 1280
#else
    // testing block sizes (single im MB, multiple in KB) which let the tests
    // run approximately 5s on my machine in the not optimized mode
    #define single_block_size    16
    #define multiple_block_size  64
    #define multiple_block_count 256
#endif

// the class name of a test suite; base64 has to be defined as a namespace
// name with an alternative implementation of the base64 encoding interface
#define test_name concatenate(base64, _test)

// the code which actually performs the encoding; here by the message class
#define encode_single_block std::string result; \
    message::base64_encode(buffer, result)

// 1. testing the code from boost/network/protocol/http/message.hpp
#define base64 base64_from_message
#include "utils_base64_experiment.ipp"
#undef base64

// change the code to use the experimental headers; base64 has to be defined
// as a namespace name - see above
#undef encode_single_block
#define encode_single_block std::string result = base64::encode<char>(buffer)

// 2. testing the code from experimental/base64-stateless.hpp
#define base64 base64_stateless
#include "utils_base64_experiment.ipp"
#undef base64

// enable the second test case, which encodes the input buffer chunk
// by chunk and remembers the encoding state to be able to continue
#define base64_with_state

// introduce the code for encoding by chunks
#define encode_multiple_blocks \
    std::string result; \
    std::back_insert_iterator<std::string> result_encoder(result); \
    base64::state<unsigned char> rest; \
    for (unsigned block_index = 0; block_index < buffers.size(); \
                                   ++block_index) { \
        std::vector<unsigned char> const & buffer = buffers[block_index]; \
        base64::encode(buffer.begin(), buffer.end(), result_encoder, rest); \
    } \
    base64::encode_rest(result_encoder, rest)

// 3. testing the code from experimental/base64-stateful_buffer.hpp
#define base64 base64_stateful_buffer
#include "utils_base64_experiment.ipp"
#undef base64

// 4. testing the code from experimental/base64-stateful_transform.hpp
#define base64 base64_stateful_transform
#include "utils_base64_experiment.ipp"
#undef base64

// 5. testing the code from experimental/base64-stateful_iterator.hpp
#define base64 base64_stateful_iterator
#include "utils_base64_experiment.ipp"
#undef base64

// 6. testing the code from experimental/base64-standalone.hpp,
// which has become the code in boost/network/utils/base64/encode.hpp
#define base64 base64_standalone
#include "utils_base64_experiment.ipp"
#undef base64

// redefine the testing code to use the updated interface from
// boost/network/utils/base64/encode.hpp which supports encoding
// customization, like custom alphabets, disabling padding and line
// breaks on a specified position
#undef encode_single_block
#undef encode_multiple_blocks
#define encode_single_block std::string result = \
    base64::encode<base64::normal, char>(buffer)
#define encode_multiple_blocks \
    std::string result; \
    std::back_insert_iterator<std::string> result_encoder(result); \
    base64::state<unsigned char> rest; \
    for (unsigned block_index = 0; block_index < buffers.size(); \
                                   ++block_index) { \
        std::vector<unsigned char> const & buffer = buffers[block_index]; \
        base64::encode<base64::normal>(buffer.begin(), buffer.end(), \
                                       result_encoder, rest); \
    } \
    base64::encode_rest<base64::normal>(result_encoder, rest)

// 7. testing the code from boost/network/utils/base64/encode.hpp
#define base64 base64
#include "utils_base64_experiment.ipp"
#undef base64

// redefine the testing code to use the iostream implementation from
// boost/network/utils/base64/encode-io.hpp which depends on the
// interface from boost/network/utils/base64/encode.hpp
#undef test_name
#undef encode_single_block
#undef encode_multiple_blocks
#define test_name concatenate(concatenate(base64, _io), _test)
#define encode_single_block std::stringstream result; \
    result << base64::io::encode<base64::normal>(buffer) << \
              base64::io::encode_rest<base64::normal, unsigned char>
#define encode_multiple_blocks \
    std::stringstream result; \
    for (unsigned block_index = 0; block_index < buffers.size(); \
                                   ++block_index) { \
        std::vector<unsigned char> const & buffer = buffers[block_index]; \
        result << base64::io::encode<base64::normal>(buffer.begin(), \
                                                     buffer.end()); \
    } \
    result << base64::io::encode_rest<base64::normal, unsigned char>

// 8, testing the iostream implementation with the code from
// boost/network/utils/base64/encode.hpp
#define base64 base64
#include "utils_base64_experiment.ipp"
#undef base64
