#include <iostream>
#include <string.h>
#include <fstream>

#include "error.hpp"
#include "lexer.hpp"
#include "parser.hpp"

int main(int argc, char *argv[]){
    if(argc == 2){
        std::string pfile = argv[1];
        if (pfile.size() > 5 && pfile.substr(pfile.size() - 5) == ".slfr"){
            std::ifstream file(pfile);
            if (!file.is_open()){
                error err;
                err.code = 3;
                err.source = "Driver";
                err.msg = "Cannot open file";
                err.hint = "Make sure the file exists and the path is correct!";
                err.file = pfile;
                printErr(err);
                return 3;
            }
            if (file.peek() == std::ifstream::traits_type::eof()){
                error err;
                err.code = 4;
                err.source = "Driver";
                err.msg = "Source file is empty";
                err.hint = "Try writing something into your source file!";
                err.file = pfile;
                printErr(err);
                return 4;
            }

            //Compile Here

        }else{
            error err;
            err.code = 2;
            err.source = "Driver";
            err.msg = "Wrong archive type";
            err.hint = "File must be a .slfr file";
            printErr(err);
            return 2;
        }
    }else{
        error err;
        err.code = 1;
        err.source = "Driver";
        err.msg = "Bad Arguments";
        err.hint = "Usage: ./slfr file.slfr";
        printErr(err);
        return 1;
    }
    return 0;
}
