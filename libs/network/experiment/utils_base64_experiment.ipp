// define these macros before including this file:
//     base64                 - namespace with the BASE64 encoding functions
//     test_name              - name of the testing class
//     single_block_size      - buffer size in MB for the single-block test
//     multiple_block_size    - buffer size in KB for the multi-block test
//     multiple_block_count   - count of the blocks for the multi-block test
//     encode_single_block    - perform encoding of a single input sequence
//     encode_multiple_blocks - perform encoding of an input sequence which
//                              consists of multiple chunks
//     base64_with_state      - enables the multple-block test, which needs
//                              a stateful encoding impleentation
// also, define these general purpose macros:
//     stringify   - make a string of a macro argument
//     concatenate - join two macro arguments to a single identifier

// testing class executing the benchmark methods in its constructor
class test_name {
    void test_single_block();
    void test_multiple_blocks();
public:
    test_name() {
        std::cout << ++test_index << ". Executing "
            stringify(test_name) ":" << std::endl;
        test_single_block();
        test_multiple_blocks();
    }
};

// construct a test instance relying on its constructor to run the benchmarks
test_name concatenate(run_, test_name);

// encode an input sequence as a single complete block
void test_name::test_single_block() {
    std::cout << "     Encoding " << single_block_size << " MB buffer took ";
    // Fill a single string with random bytes.
    std::string buffer(single_block_size * 1024 * 1024, 0);
    for (std::string::iterator current = buffer.begin();
          current != buffer.end(); ++current)
        *current = static_cast<char>(rand() % 255);
    // Encode the single string to a single BASE64 string.
    clock_t start = clock();
    encode_single_block;
    clock_t end = clock();
    std::cout << (double) (end - start) / CLOCKS_PER_SEC <<
                  "s." << std::endl;
}

// encode an input sequence chunk by chunk
void test_name::test_multiple_blocks() {
#ifdef base64_with_state
    std::cout << "     Encoding " << multiple_block_count << " x " <<
        multiple_block_size << " KB buffers took ";
    // Fill multiple vectors with random bytes.
    std::vector<std::vector<unsigned char> > buffers(multiple_block_count);
    for (unsigned block_index = 0; block_index < buffers.size();
                                     ++block_index) {
        std::vector<unsigned char> & buffer = buffers[block_index];
        buffer.resize(multiple_block_size * 1024);
        for (std::vector<unsigned char>::iterator current = buffer.begin();
              current != buffer.end(); ++current)
            *current = static_cast<unsigned char>(rand() % 255);
    }
    // Encode the multiple vectors to a single BASE64 string.
    clock_t start = clock();
    encode_multiple_blocks;
    clock_t end = clock();
    std::cout << (double) (end - start) / CLOCKS_PER_SEC <<
                  "s." << std::endl;
#endif
}
