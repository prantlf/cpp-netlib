#ifndef BOOST_NETWORK_UTILS_BASE64_ENCODE_HPP
#define BOOST_NETWORK_UTILS_BASE64_ENCODE_HPP

#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>
#include <algorithm>
#include <iterator>
#include <string>

namespace boost {
namespace network {
namespace utils {

// Implements a BASE64 converter working on an iterator range.  If the input
// consists of multiple chunks, stateful method overloads preserve the
// encoding state to resume the encoding as if the input sequence was in one
// piece.  The encoding parameters (target alphabet, disabling padding, line
// breaks on a specified position) can be set by the template parameters.
// Real-world encoding options specified by the RFC 4648 are provided as
// template specializations.
//
// Summarized interface:
//
// struct state<Value>  {
//     bool empty () const;
//     void clear();
// }
//
// typedef struct encoder<Alphabet, Padding, MaxLineLength> :
//     normal, url, mime, pem
//
// OutputIterator encode(InputIterator begin,
//                       InputIterator end,
//                       OutputIterator output,
//                       State & rest)
// OutputIterator encode_rest(OutputIterator output,
//                            State & rest)
// OutputIterator encode(InputRange const & input,
//                       OutputIterator output,
//                       State & rest)
// OutputIterator encode(char const * value,
//                       OutputIterator output,
//                       state<char> & rest)
// std::basic_string<Char> encode(InputRange const & value,
//                                State & rest)
// std::basic_string<Char> encode(char const * value,
//                                state<char> & rest)
//
// OutputIterator encode(InputIterator begin, InputIterator end,
//                       OutputIterator output)
// OutputIterator encode(InputRange const & input,
//                       OutputIterator output)
// OutputIterator encode(char const * value,
//                       OutputIterator output)
// std::basic_string<Char> encode(InputRange const & value)
// std::basic_string<Char> encode(char const * value)
//
// See http://tools.ietf.org/html/rfc4648 for the specification.
// See also http://libb64.sourceforge.net, which served as inspiration.

namespace base64 {

// ENCODING STATE: Storage
// -----------------------

// Stores the state after processing the last chunk by the encoder.  If the
// chunk byte-length is not divisible by three, the last (incomplete) value
// quantum canot be encoded right away; it has to wait for the next chunk
// of octets which will be processed as if the previous one continued with
// it.  The state remembers what is necessary to resume the encoding.
template <typename Value>
struct state {
    state() : triplet_index(0), last_encoded_value(0), line_length(0) {}

    state(state const & source)
        : triplet_index(source.triplet_index),
          last_encoded_value(source.last_encoded_value),
          line_length(source.line_length) {}

    bool empty() const { return triplet_index == 0; }

    void clear() {
        // indicate that no rest has been left in the last encoded value
        // and no padding is needed for the encoded output
        triplet_index = 0;
        // the last encoded value, which may have been left from the last
        // encoding step, must be zeroed too; it is important before the
        // next encoding begins, because it works as a cyclic buffer and
        // must start empty - with zero
        last_encoded_value = 0;
        // reset the current line length too
        line_length = 0;
    }
  
protected:
    // helper to set the encoding state in this instance
    void set(unsigned char index, Value value, unsigned short length) {
        triplet_index = index;
        last_encoded_value = value;
        line_length = length;
    }

    // Keep the size of this structure as small as possible.  Descenants
    // for special purposes may need to serialize it to a binary buffer
    // of a limited size.  For example, an integration to iostreams may
    // want to store in the stream internal extensible array which is
    // a long variable and the minimum long size is 4 bytes.

    // number of the octet in the incomplete quantum, which has been
    // processed the last time; 0 means that the previous quantum was
    // complete 3 octets, 1 that just one octet was avalable and 2 that
    // two octets were available
    unsigned char triplet_index;
    // the value made of the previously shifted and or-ed octets which
    // was not completely split to 6-bit codes, because the last quantum
    // did not stop on the boundary of three octets
    Value last_encoded_value;
    // the length of the current line written to the encoded output
    // which tracks the position
    unsigned short line_length;

    // encoding of an input chunk needs to read and update the state
    template <
        typename Alphabet,
        typename Padded,
        unsigned short MaxLineLength
        >
    friend struct encoder;

    // padding needs to now how many 6-bit units were missing
    friend struct padding;
};

// ENCODING OPTIONS: Alphabets to translate the encoded 6-bit code units to
// ------------------------------------------------------------------------

// Defines the default BASE64 encoding alphabet.  Characters 62 and 63
// are '+' and '/'.
struct default_alphabet {
    // Picks a character from the output alphabet for an encoded 6-bit value.
    template <typename Value>
    static char translate(Value value) {
        static char const * characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "0123456789+/";
        return characters[static_cast<unsigned>(value)];
    }

private:
    // disallow construction; this is a static class
    default_alphabet();
    default_alphabet(default_alphabet const &);
};

// Defines the BASE64 encoding alphabet which output is considered "safe"
// to be a part of a URL (no need to be URL-encoded) or of a file name
// (no invalid characters).  Characters 62 and 63 are '-' and '_'.
struct url_and_filename_safe_alphabet {
    // Picks a character from the output alphabet for an encoded 6-bit value.
    template <typename Value>
    static char translate(Value value) {
        static char const * characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "0123456789-_";
        return characters[static_cast<unsigned>(value)];
    }

private:
    // disallow construction; this is a static class
    url_and_filename_safe_alphabet();
    url_and_filename_safe_alphabet(url_and_filename_safe_alphabet const &);
};

// ENCODING OPTIONS: Padding the encoded output with the '=' characters
// --------------------------------------------------------------------

// Implements padding of the encoded output with the '=' characters if the
// input sequence didn't end on the complete encoding quantum boundary
// (it's byte-length was not divisible by three).
struct padding {
    // Appends one or two '=' characters if the remainder after dividing the
    // byte-length of the input sequence was two or one respectively.
    template <
        typename OutputIterator,
        typename State
        >
    static OutputIterator append_to(OutputIterator output,
                                    State & rest) {
        if (!rest.empty()) {
            // at least one padding '=' will be always needed - at least two
            // bits are missing in the finally encoded 6-bit value
            *output++ = '=';
            // if the last octet was the first in the triplet (the index was
            // 1), four bits are missing in the finally encoded 6-bit value;
            // another '=' character is needed for the another two bits
            if (rest.triplet_index < 2)
                *output++ = '=';
        }
        return output;
    }

private:
    // disallow construction; this is a static class
    padding();
    padding(padding const &);
};

// Implements the padding interface by no output to support encoding without
// padding - when the encoded buffer length is expressed by other means.
struct no_padding : public padding {
    // Writes intentionally nothing out.
    template <
        typename OutputIterator,
        typename State
        >
    static OutputIterator append_to(OutputIterator output,
                                    State & rest) {
        return output;
    }

private:
    // disallow construction; this is a static class
    no_padding();
    no_padding(no_padding const &);
};

// ENCODING OPTIONS: Line breaks after every specified character count
// -------------------------------------------------------------------

// Declares constants for maximum line lengths required by the BASE64
// specification (RFC 4648) for common encoding scenarios.
enum {
    // Zero means inserting no line breaks to the encoded output.
    no_line_breaks = 0,
    // Multipurpose Internet Mail Extensions (MIME) enforces a limit
    // on line length of BASE64-encoded data to 76 characters.
    max_mime_line_length = 76,
    // Privacy Enhanced Mail (PEM) enforces a limit on line length
    // of BASE64-encoded data to 64 characters.
    max_pem_line_length = 64
};

// ENCODING OPTIONS: The static encoder class template
// ---------------------------------------------------

// Encapsulates static encoding methods and controls their behaviour by
// the template parameters to support various scenarios described in the
// BASE64 specification (RFC 4648).
template <
    typename Alphabet,
    typename Padding,
    unsigned short MaxLineLength
    >
struct encoder {
    // Alphabet to translate the 6-bit encoded code units to.
    typedef Alphabet alphabet;
    // Implementation of the encoded output padding.
    typedef Padding padding;
    // The maximum line length of the encoded output.
    enum { max_line_length = MaxLineLength };

    // STATEFUL ENCODING CORE: Continuing and finishing functions
    // ----------------------------------------------------------

    // Encodes an input sequence to BASE64 writing it to the output iterator
    // and stopping if the last input tree-octet quantum was not complete, in
    // which case it stores the state for the later continuation, when another
    // input chunk is ready for the encoding.  The encoding must be finished
    // by calling the encode_rest after processing the last chunk.
    //
    // std::vector<unsigned char> buffer = ...;
    // std::basic_string<Char> result;
    // std::back_insert_iterator<std::basic_string<Char> > appender(result);
    // base64::state<unsigned char> rest;
    // base64::normal::encode(buffer.begin(), buffer.end(), appender, rest);
    // ...
    // base64::normal::encode_rest(appender, rest);
    template <
        typename InputIterator,
        typename OutputIterator,
        typename State
        >
    static OutputIterator encode(InputIterator begin,
                                 InputIterator end,
                                 OutputIterator output,
                                 State & rest) {
        typedef typename iterator_value<InputIterator>::type value_type;
        // continue with the rest of the last chunk - 2 or 4 bits which
        // are already shifted to the left and need to be or-ed with the
        // continuing data up to the target 6 bits
        value_type encoded_value = rest.last_encoded_value;
        unsigned short line_length = rest.line_length;
        // if the previous chunk stopped at encoding the first (1) or the
        // second (2) octet of the three-byte quantum, jump to the right
        // place, otherwise start the loop with an empty encoded value buffer
        switch (rest.triplet_index) {
            // this loop processes the input sequence of bit-octets by bits,
            // shifting the current_value (used as a cyclic buffer) left and
            // or-ing next bits there, while pulling the bit-sextets from the
            // high word of the current_value
            for (value_type current_value;;) {
        case 0:
                // if the input sequence is empty or reached its end at the
                // 3-byte boundary, finish with an empty encoding state
                if (begin == end) {
                    // the last encoded value is not interesting - it would
                    // not be used, because processing of the next chunk will
                    // start at the 3-byte boundary
                    rest.set(0, 0, line_length);
                    return output;
                }
                // read the first octet from the current triplet
                current_value = *begin++;
                // use just the upper 6 bits to encode it to the target alphabet
                encoded_value = (current_value & 0xfc) >> 2;
                *output++ = alphabet::translate(encoded_value);
                // shift the remaining two bits up to make place for the
                // upoming part of the next octet
                encoded_value = (current_value & 0x03) << 4;
        case 1:
                // if the input sequence reached its end after the first octet
                // from the quantum triplet, store the encoding state and finish
                if (begin == end) {
                    rest.set(1, encoded_value, line_length);
                    return output;
                }
                // read the second first octet from the current triplet
                current_value = *begin++;
                // combine the upper four bits (as the lower part) with the
                // previous two bits to encode it to the target alphabet
                encoded_value |= (current_value & 0xf0) >> 4;
                *output++ = alphabet::translate(encoded_value);
                // shift the remaining four bits up to make place for the
                // upoming part of the next octet
                encoded_value = (current_value & 0x0f) << 2;
        case 2:
                // if the input sequence reached its end after the second octet
                // from the quantum triplet, store the encoding state and finish
                if (begin == end) {
                    rest.set(2, encoded_value, line_length);
                    return output;
                }
                // read the third octet from the current triplet
                current_value = *begin++;
                // combine the upper two bits (as the lower part) with the
                // previous four bits to encode it to the target alphabet
                encoded_value |= (current_value & 0xc0) >> 6;
                *output++ = alphabet::translate(encoded_value);
                // encode the remaining 6 bits to the target alphabet
                encoded_value  = current_value & 0x3f;
                *output++ = alphabet::translate(encoded_value);

                // Zero maximum line length means no line breaks in the output.
                if (max_line_length) {
                    // another four 6-bit units were written to the output
                    line_length += 4;
                    // if the maximum line legth was reached or exceeded, append
                    // a line break and reset the line length register
                    if (line_length >= max_line_length) {
                        *output++ = '\n';
                        line_length = 0;
                    }
                }
            }
        }
        return output;
    }

    // Finishes encoding of the previously processed chunks.  If their total
    // byte-length was divisible by three, nothing is needed; if not, the
    // last quantum will be encoded as if padded with zeroes, which will be
    // indicated by appending '=' characters as padding to the output.  This
    // method must be always used at the end of encoding performed the method
    // overloads using the encoding state.
    //
    // std::vector<unsigned char> buffer = ...;
    // std::basic_string<Char> result;
    // std::back_insert_iterator<std::basic_string<Char> > appender(result);
    // base64::state<unsigned char> rest;
    // base64::normal::encode(buffer.begin(), buffer.end(), appender, rest);
    // ...
    // base64::normal::encode_rest(appender, rest);
    template <
        typename OutputIterator,
        typename State
        >
    static OutputIterator encode_rest(OutputIterator output,
                                      State & rest) {
        if (!rest.empty()) {
            // process the last part of the trailing octet (either 4 or 2 bits)
            // as if the input was padded with zeros - without or-ing the next
            // input value to it; it has been already shifted to the left
            *output++ = alphabet::translate(rest.last_encoded_value);
            // append padding for the incomplete last quantum as necessary
            output = padding::append_to(output, rest);
            // clear the state all the time to make sure that another call to
            // the encode_rest would not cause damage; the last encoded value,
            // which may have been left there, must be zeroed too; it is
            // important before the next encoding begins, because it works as
            // a cyclic buffer and must start empty - with zero
            rest.clear();
        }
        return output;
    }

    // STATEFUL ENCODING: Methods for a chunked input
    // ----------------------------------------------

    // Encodes an entire input sequence to BASE64, which either supports begin()
    // and end() methods returning boundaries of the sequence or the boundaries
    // can be computed by the Boost::Range, writing it to the output iterator
    // and stopping if the last input tree-octet quantum was not complete, in
    // which case it stores the state for the later continuation, when another
    // input chunk is ready for the encoding.  The encoding must be finished
    // by calling the encode_rest after processing the last chunk.
    //
    // Warning: Buffers passed in as C-pointers are processed including their
    // termination character, if they have any.  This is unexpected at least
    // for the storing literals, which have a specialization here to avoid it.
    //
    // std::vector<unsigned char> buffer = ...;
    // std::basic_string<Char> result;
    // std::back_insert_iterator<std::basic_string<Char> > appender(result);
    // base64::state<unsigned char> rest;
    // base64::normal::encode(buffer, appender, rest);
    // ...
    // base64::normal::encode_rest(appender, rest);
    template <
        typename InputRange,
        typename OutputIterator,
        typename State
        >
    static OutputIterator encode(InputRange const & input,
                                 OutputIterator output,
                                 State & rest) {
        return encode(boost::begin(input), boost::end(input),
                      output, rest);
    }

    // Encodes an entire string literal to BASE64, writing it to the output
    // iterator and stopping if the last input tree-octet quantum was not
    // complete, in which case it stores the state for the later continuation,
    // when another input chunk is ready for the encoding.  The encoding must
    // be finished by calling the encode_rest after processing the last chunk.
    //
    // The string literal is encoded without processing its terminating zero
    // character, which is the usual expectation.
    //
    // std::basic_string<Char> result;
    // std::back_insert_iterator<std::basic_string<Char> > appender(result);
    // base64::state<char> rest;
    // base64::normal::encode("ab", appender, rest);
    // ...
    // base64::normal::encode_rest(appender, rest);
    template <typename OutputIterator>
    static OutputIterator encode(char const * value,
                                 OutputIterator output,
                                 state<char> & rest) {
        return encode(value, value + strlen(value), output, rest);
    }

    // STATELESS ENCODING: Methods for an input available in a single piece
    // --------------------------------------------------------------------

    // Encodes a part of an input sequence specified by the pair of begin and
    // end iterators.to BASE64 writing it to the output iterator. If its total
    // byte-length was not divisible by three, the output will be padded by
    // the '=' characters.  If you encode an input consisting of mutiple
    // chunks, use the method overload maintaining the encoding state.
    //
    // std::vector<unsigned char> buffer = ...;
    // std::basic_string<Char> result;
    // base64::normal::encode(buffer.begin(), buffer.end(),
    //                               std::back_inserter(result));
    template <
        typename InputIterator,
        typename OutputIterator
        >
    static OutputIterator encode(InputIterator begin,
                                 InputIterator end,
                                 OutputIterator output) {
        state<typename iterator_value<InputIterator>::type> rest;
        output = encode(begin, end, output, rest);
        return encode_rest(output, rest);
    }

    // Encodes an entire input sequence to BASE64 writing it to the output
    // iterator, which either supports begin() and end() methods returning
    // boundaries of the sequence or the boundaries can be computed by the
    // Boost::Range. If its total byte-length was not divisible by three,
    // the output will be padded by the '=' characters.  If you encode an
    // input consisting of mutiple chunks, use the method overload maintaining
    // the encoding state.
    //
    // Warning: Buffers passed in as C-pointers are processed including their
    // termination character, if they have any.  This is unexpected at least
    // for the storing literals, which have a specialization here to avoid it.
    //
    // std::vector<unsigned char> buffer = ...;
    // std::basic_string<Char> result;
    // base64::normal::encode(buffer, std::back_inserter(result));
    template <
        typename InputRange,
        typename OutputIterator
        >
    static OutputIterator encode(InputRange const & value,
                                 OutputIterator output) {
        return encode(boost::begin(value), boost::end(value), output);
    }

    // Encodes an entire input sequence to BASE64 returning the result as
    // string, which either supports begin() and end() methods returning
    // boundaries of the sequence or the boundaries can be computed by the
    // Boost::Range. If its total byte-length was not divisible by three,
    // the output will be padded by the '=' characters.  If you encode an
    // input consisting of mutiple chunks, use other method maintaining
    // the encoding state writing to an output iterator.
    //
    // Warning: Buffers passed in as C-pointers are processed including their
    // termination character, if they have any.  This is unexpected at least
    // for the storing literals, which have a specialization here to avoid it.
    //
    // std::vector<unsigned char> buffer = ...;
    // std::basic_string<Char> result = base64::normal::encode<Char>(buffer);
    template <
        typename Char,
        typename InputRange
        >
    static std::basic_string<Char> encode(InputRange const & value) {
        std::basic_string<Char> result;
        encode(value, std::back_inserter(result));
        return result;
    }

    // The function overloads for string literals encode the input without
    // the terminating zero, which is usually expected, because the trailing
    // zero byte is not considered a part of the string value; the overloads
    // for an input range would wrap the string literal by Boost.Range and
    // encode the full memory occupated by the string literal - including the
    // unwanted last zero byte.

    // Encodes an entire string literal to BASE64 writing it to the output
    // iterator. If its total length (without the trailing zero) was not
    // divisible by three, the output will be padded by the '=' characters.
    // If you encode an input consisting of mutiple chunks, use the method
    // overload maintaining the encoding state.
    //
    // The string literal is encoded without processing its terminating zero
    // character, which is the usual expectation.
    //
    // std::basic_string<Char> result;
    // base64::normal::encode("ab", std::back_inserter(result));
    template <typename OutputIterator>
    static OutputIterator encode(char const * value,
                                 OutputIterator output) {
        return encode(value, value + strlen(value), output);
    }

    // Encodes an entire string literal to BASE64 returning the result as
    // string. If its total byte-length was not divisible by three, the
    // output will be padded by the '=' characters.  If you encode an
    // input consisting of mutiple chunks, use other method maintaining
    // the encoding state writing to an output iterator.
    //
    // The string literal is encoded without processing its terminating zero
    // character, which is the usual expectation.
    //
    // std::basic_string<Char> result = base64::normal::encode<Char>("ab");
    template <typename Char>
    static std::basic_string<Char> encode(char const * value) {
        std::basic_string<Char> result;
        encode(value, std::back_inserter(result));
        return result;
    }

private:
    // Zero means inserting no line breaks to the encoded output,
    // less than zero doesn't make sense.
    BOOST_STATIC_ASSERT(MaxLineLength >= 0);

    // disallow construction; this is a static class
    encoder() {}
    encoder(encoder const &) {}
};

// ENCODING OPTIONS: The encoder specializations for the most common
//                   scenarios described in the RFC 4648
// -----------------------------------------------------------------

// The default BASE64 encoding.  Padded with the '=' characters if
// necessary, no line breaks inserted.
typedef struct encoder<
    default_alphabet,
    padding,
    no_line_breaks
    > normal;

// The BASE64 encoding using an alternative alphabet, which ensures that the
// encoded output can be used in URLs and file names.  Padded with the '='
// characters if necessary, no line breaks inserted.  This encoding is
// sometimes called "base64url".
typedef struct encoder<
    url_and_filename_safe_alphabet,
    padding,
    no_line_breaks
    > url;

// The BASE64 encoding for content parts wrapped in the envelope of the
// Multipurpose Internet Mail Extensions (MIME).  Padded with the '='
// characters if necessary, a line break after every 76 character.
typedef struct encoder<
    default_alphabet,
    padding,
    max_mime_line_length
    > mime;

// The BASE64 encoding for content parts wrapped in the envelope of the
// Privacy Enhanced Mail (PEM).  Padded with the '=' characters if
// necessary, a line break after every 64 character.
typedef struct encoder<
    default_alphabet,
    padding,
    max_pem_line_length
    > pem;

} // namespace base64

} // namespace utils
} // namespace network
} // namespace boost

#endif // BOOST_NETWORK_UTILS_BASE64_ENCODE_HPP
