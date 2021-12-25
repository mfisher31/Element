
#include <boost/wave/cpp_context.hpp>
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp>

namespace wave = boost::wave;

int main()
{
    const char* filename = "input.cpp";
    std::ifstream instream (filename);
    std::string input (std::istreambuf_iterator<char> (instream.rdbuf()),
                       std::istreambuf_iterator<char>());

    // The template boost::wave::cpplexer::lex_token<> is the
    // token type to be used by the Wave library.
    // This token type is one of the central types throughout
    // the library, because it is a template parameter to some
    // of the public classes and templates and it is returned
    // from the iterators.
    // The template boost::wave::cpplexer::lex_iterator<> is
    // the lexer iterator to use as the token source for the
    // preprocessing engine. In this case this is parameterized
    // with the token type.
    using lex_iterator_type = wave::cpplexer::lex_iterator<wave::cpplexer::lex_token<>>;
    using context_type = wave::context<std::string::iterator, lex_iterator_type>;

    context_type ctx (input.begin(), input.end(), filename);

    // At this point you may want to set the parameters of the
    // preprocessing as include paths and/or predefined macros.
    ctx.add_include_path (".");
    // ctx.add_macro_definition("_ELEMENT");

    // Get the preprocessor iterators and use them to generate
    // the token sequence.
    auto first = ctx.begin();
    auto last = ctx.end();

    try {
        // The input stream is preprocessed for you during iteration
        // over [first, last)
        while (first != last) {
            boost::wave::token_id tid = (*first);

            if (tid == boost::wave::T_STRUCT) {
                std::cout << (*first).get_value();
                
            } else {
                std::cout << "NOT STRUCT: " << (*first).get_value();
            }

            ++first;
        }
    } catch (const wave::preprocess_exception& e) {
        std::clog << e.description() << std::endl;
    }
    return 0;
}
