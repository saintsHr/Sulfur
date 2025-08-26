#include "parser.hpp"
#include "error.hpp"

Parser::Parser(std::vector<Token>& toks) : tokens(toks), current(0) {}