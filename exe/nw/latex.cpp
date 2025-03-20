////////////////////////////////////////////////////////////////////////////////
//
//  YOUR QUILL
//
////////////////////////////////////////////////////////////////////////////////

#include "common.hpp"

struct Latex : public Formatter {
    Latex()
    {
        sSuffix = ".tex";
        formats.insert("latex");
    }
    
    virtual ~Latex()
    {
    }
};

Formatter*  fmtLatex()
{
    return new Latex;
}

